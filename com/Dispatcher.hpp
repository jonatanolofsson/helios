#ifndef OS_COM_DISPATCHER_HPP_
#define OS_COM_DISPATCHER_HPP_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <os/com/com.hpp>
#include <os/com/Signal.hpp>
#include <os/exceptions.hpp>
#include <iostream>

namespace os {
    template<typename T> class Signal;
    template<typename T> Signal<T>& getSignal();

    template<typename T>
    void yield(const T& val) {
        getSignal<T>() = val;
    }

    template<typename T>
    class Via {
        private:
            Signal<T>& signal;
            int lastId;
            bool dying;
        protected:
            Via() : signal(getSignal<T>()), lastId(0), dying(false) {}
            ~Via() {
                halt();
            }
            void halt() {
                dying = true;
                signal.cond.notify_all();
            }
            const T value() {
                std::unique_lock<std::mutex> l(signal.guard);
                while(lastId == signal.id && !dying) signal.cond.wait(l);
                if(dying) throw os::HaltException();
                lastId = signal.id;
                return *signal;
            }
    };

    enum DispatcherActivityControl {
        DAC_ACTIVATING,
        DAC_DEACTIVATING,
        DAC_QUERY,
        DAC_WAIT,
        DAC_DIE,
        DAC_RESET
    };

    int activeDispatcherActions(DispatcherActivityControl);
    template<typename T, typename... TARG>
    class Dispatcher : public Via<TARG>... {
        public:
            typedef void(T::*ActionFunction)(TARG...);
            Dispatcher(const ActionFunction& fn, T* const that_)
                : Via<TARG>()...
                , that(that_)
                , action(fn)
                , invokations(0)
                , dying(false)
            { t = std::thread(&Dispatcher<T, TARG...>::run, this); }
            ~Dispatcher() {
                join();
            }
            void wait(const int last_invokation) {
                std::unique_lock<std::mutex> l(invokationGuard);
                while(last_invokation > invokations && !dying) cond.wait(l);
            }
            int getInvokations() {
                std::unique_lock<std::mutex> l(invokationGuard);
                return invokations;
            }
            void join() {
                dying = true;
                cond.notify_all();
                HaltVia<TARG...>::halt(this);
                t.join();
            }
            void join_next() {
                dying = true;
            }
        private:
            template<typename... TYPES>
            struct HaltVia {static void halt(Dispatcher<T, TARG...>*){}};
            template<typename T0, typename... TYPES>
            struct HaltVia<T0, TYPES...> {
                static void halt(Dispatcher<T, TARG...>* obj) {
                    obj->Via<T0>::halt();
                    HaltVia<TYPES...>::halt(obj);
                }
            };

            std::mutex invokationGuard;
            std::condition_variable cond;
            T* const that;
            const ActionFunction action;
            int invokations;
            bool dying;
            std::thread t;
            void run() {
                try {
                    while(!dying) {
                        performAction(Via<TARG>::value()...);
                        std::unique_lock<std::mutex> l(invokationGuard);
                        ++invokations;
                        cond.notify_all();
                    }
                }
                catch (os::HaltException& e) {}
                catch (std::exception& e) {
                    std::cerr << e.what();
                }
            }

            void performAction(TARG... args) {
                activeDispatcherActions(DAC_ACTIVATING);
                (that->*action)(args...);
                activeDispatcherActions(DAC_DEACTIVATING);
            }
    };
}

#endif
