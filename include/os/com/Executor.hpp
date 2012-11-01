#ifndef OS_COM_EXECUTOR_HPP_
#define OS_COM_EXECUTOR_HPP_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/arithmetic.hpp>
#include <os/com/com.hpp>

namespace os {
    namespace com {
        template<typename T> class Signal;
        template<typename T> Signal<T>& getSignal();

        template<typename T>
        class Signal {
            public:
                T value;
                std::mutex guard;
                std::condition_variable cond;
                int id;
                Signal() : id(0) {};
            public:
                void operator=(const T& val) {
                    std::lock_guard<std::mutex> l(guard);
                    value = val;
                    ++id;
                    cond.notify_all();
                }
                const T operator*() {
                    return value;
                }
            template<typename TT>
            friend Signal<TT>& getSignal();
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
            protected:
                Via() : signal(getSignal<T>()), lastId(0) {}
                const T value() {
                    std::unique_lock<std::mutex> l(signal.guard);
                    while(lastId == signal.id) signal.cond.wait(l);
                    lastId = signal.id;
                    return *signal;
                }
        };

        template<typename... TARG>
        class Executor : Via<TARG>... {
            public:
                typedef void(*actionfunction_t)(TARG...);
                Executor(const actionfunction_t& fn, bool singleRun = false)
                    : Via<TARG>()...
                    , action(fn)
                    , invokations(0)
                    , running(!singleRun)
                { t = std::thread(&Executor<TARG...>::run, this); }
                void wait(const int invokation) {
                    std::unique_lock<std::mutex> l(invokationGuard);
                    while(invokation > invokations) cond.wait(l);
                }
                int getInvokations() {
                    std::unique_lock<std::mutex> l(invokationGuard);
                    return invokations;
                }
                void join() {
                    running = false;
                    t.join();
                }
                void join_next() {
                    running = false;
                }
            private:
                std::mutex invokationGuard;
                std::condition_variable cond;
                actionfunction_t action;
                int invokations;
                bool running;
                std::thread t;
                void run() {
                    do {
                        action(Via<TARG>::value()...);
                        std::unique_lock<std::mutex> l(invokationGuard);
                        ++invokations;
                        cond.notify_all();
                    } while(running);
                }
        };
    }
}

#endif
