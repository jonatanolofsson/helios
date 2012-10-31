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
            public:
                Signal<T>& signal;
                int lastId;
                Via() : signal(getSignal<T>()), lastId(0) {}
            public:
                const T operator*() {
                    std::unique_lock<std::mutex> l(signal.guard);
                    while(lastId == signal.id) signal.cond.wait(l);
                    lastId = signal.id;
                    return *signal;
                }
        };

        #define sigdecl(z,n,q) Via<TARG ## n> arg ## n;
            #define voidarg(z,n,q) BOOST_PP_COMMA_IF(n) void
                #define TMPLARGS(N) BOOST_PP_ENUM_PARAMS(N, TARG)                           \
                    BOOST_PP_REPEAT_FROM_TO(N, MAX_EXECUTOR_PARAMS, voidarg,)
                    #define MAKE_EXECUTOR(z,N,q)                                            \
                        template<BOOST_PP_ENUM_PARAMS(N, typename TARG)>                    \
                        class Executor<TMPLARGS(N)> {                                       \
                            public:                                                         \
                                typedef void(*actionfunction_t)(BOOST_PP_ENUM_PARAMS(N, const TARG));  \
                                Executor(const actionfunction_t& fn, bool singleRun = false)\
                                    : action(fn)                                            \
                                    , invokations(0)                                        \
                                    , running(!singleRun)                                   \
                                { t = std::thread(&Executor<TMPLARGS(N)>::run, this); }     \
                                void wait(const int lastInvokation) {                       \
                                    std::unique_lock<std::mutex> l(invokationGuard);        \
                                    while(lastInvokation == invokations) cond.wait(l);      \
                                }                                                           \
                                int getInvokations() {                                      \
                                    std::unique_lock<std::mutex> l(invokationGuard);        \
                                    return invokations;                                     \
                                }                                                           \
                                void join() {                                               \
                                    running = false;                                        \
                                    t.join();                                               \
                                }                                                           \
                                void join_next() {                                          \
                                    running = false;                                        \
                                }                                                           \
                            private:                                                        \
                                std::mutex invokationGuard;                                 \
                                std::condition_variable cond;                               \
                                actionfunction_t action;                                    \
                                int invokations;                                            \
                                bool running;                                               \
                                BOOST_PP_REPEAT(N, sigdecl, ~)                              \
                                std::thread t;                                              \
                                void run() {                                                \
                                    do {                                                    \
                                        action(BOOST_PP_ENUM_PARAMS(N, *arg));              \
                                        std::unique_lock<std::mutex> l(invokationGuard);    \
                                        ++invokations;                                      \
                                        cond.notify_all();                                  \
                                    } while(running);                                       \
                                }                                                           \
                        };

                        #define voiddef(z,n,q) BOOST_PP_COMMA_IF(n) typename TARG ## n = void
                        template<BOOST_PP_REPEAT(MAX_EXECUTOR_PARAMS, voiddef,)> class Executor;
                        #undef voiddef
                        BOOST_PP_REPEAT_FROM_TO(1, MAX_EXECUTOR_PARAMS, MAKE_EXECUTOR,)
                        #undef MAKE_EXECUTOR
                #undef TMPLARGS
            #undef voidarg
        #undef sigdecl
    }
}

#endif
