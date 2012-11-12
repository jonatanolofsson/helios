#ifndef OS_COM_EXECUTOR_HPP_
#define OS_COM_EXECUTOR_HPP_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <os/com/com.hpp>
#include <os/exceptions.hpp>

namespace os {
    namespace com {
        template<typename T> class Signal;
        template<typename T> Signal<T>& getSignal();

        void running(const int isRunning);

        template<typename T>
        class Signal {
            public:
                T value;
                std::mutex guard;
                std::condition_variable cond;
                int id;
                Signal() : id(0) {};
                void operator=(const T& val) {
                    std::lock_guard<std::mutex> l(guard);
                    value = val;
                    ++id;
                    cond.notify_all();
                }
                const T operator*() {
                    return value;
                }
        };

        template<typename T>
        Signal<T>& getSignal() {
            static Signal<T> signal;
            return signal;
        }

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

        template<typename... TARG>
        class Executor : public Via<TARG>... {
            public:
                typedef void(*ActionFunction)(TARG...);
                Executor(const ActionFunction& fn)
                    : Via<TARG>()...
                    , action(fn)
                    , invokations(0)
                    , dying(false)
                { t = std::thread(&Executor<TARG...>::run, this); }
                ~Executor() {
                    join();
                }
                void wait(const int invokation) {
                    std::unique_lock<std::mutex> l(invokationGuard);
                    while(invokation > invokations && !dying) cond.wait(l);
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
                struct HaltVia {static void halt(Executor<TARG...>*){}};
                template<typename T0, typename... TYPES>
                struct HaltVia<T0, TYPES...> {
                    static void halt(Executor<TARG...>* that) {
                        that->Via<T0>::halt();
                        HaltVia<TYPES...>::halt(that);
                    }
                };

                std::mutex invokationGuard;
                std::condition_variable cond;
                ActionFunction action;
                int invokations;
                bool dying;
                std::thread t;
                void run() {
                    try {
                        while(!dying) {
                            action(Via<TARG>::value()...);
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
        };
    }
}

#endif
