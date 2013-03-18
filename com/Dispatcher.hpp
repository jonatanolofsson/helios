#pragma once
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
    void dispatcherActionCounter(const int);

    template<typename T>
    void yield(const T& val) {
        dispatcherActionCounter(1);
        getSignal<T>().push(val);
    }

    template<typename T>
    class Via {
        private:
            Signal<T>& signal;
            bool dying;
        protected:
            Via() : signal(getSignal<T>()), dying(false) {}
            ~Via() {
                halt();
            }
            void halt() {
                dying = true;
                signal.notify_all();
            }
            const T value() {
                return signal.nextValue(dying);
            }
    };

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
                catch (const os::HaltException& e) {}
                catch (std::exception& e) {
                    std::cerr << e.what();
                }
            }

            void performAction(TARG... args) {
                (that->*action)(args...);
                dispatcherActionCounter(-(int)sizeof...(args));
            }
    };
}

#endif
