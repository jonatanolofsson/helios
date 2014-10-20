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
#include <os/utils/eventlog.hpp>
#include <os/types.hpp>
#include <os/clock.hpp>
#include <os/type_traits.hpp>

namespace os {
    namespace internal {
        template<typename T> int typedCounter(const int);
    }

    using namespace internal;
    void stopTime();
    void startTime();
    JiffyType getCurrentTick();
    void expect();
    void gotExpected();

    template<typename T>
    void yield(const T& val) {
        if(typedCounter<T>(0) > 0) {
            LOG_EVENT(typeid(T).name(), 0, "Yielded value");
            getSignal<T>().push(val);
        }
        else
        {
            LOG_EVENT(typeid(T).name(), 0, "Threw away value");
        }
    }

    namespace internal {
        extern std::mutex initGuard;
        extern std::condition_variable initCondition;
        extern int initializingDispatchers;
        extern std::condition_variable allDispatchersInitialized;

        template<typename T>
        int typedCounter(const int c) {
            static int count = 0;
            static std::mutex guard;
            std::unique_lock<std::mutex> l(guard);
            count += c;
            return count;
        }

        template<bool ASYNC> struct SynchronousDispatcherCounter {
            typedef SynchronousDispatcherCounter<ASYNC> Self;
            SynchronousDispatcherCounter() { up(); }
            ~SynchronousDispatcherCounter() { down(); }
            static unsigned int count() { return typedCounter<Self>(0); }
            static void up() { typedCounter<Self>(1); }
            static void down() { typedCounter<Self>(-1); }
        };
        template<> struct SynchronousDispatcherCounter<true> {
            static void up() {}
            static void down() {}
        };

        void activeDispatcherCounter(const int);
        template<bool ASYNC> struct ActiveDispatcherCounter {
            ~ActiveDispatcherCounter() { activeDispatcherCounter(-1); }
        };
        template<> struct ActiveDispatcherCounter<true> {
            ActiveDispatcherCounter() { activeDispatcherCounter(1); }
            ~ActiveDispatcherCounter() { activeDispatcherCounter(-1); }
        };

        template<typename T>
        class Via {
            public:
                typedef Via<T> Self;
            private:
                Signal<T>& signal;
                typename Signal<T>::SubCircle subCircle;
                bool dying;
            protected:
                Via()
                : signal(getSignal<T>())
                , dying(false)
                {
                    LOG_EVENT(typeid(Self).name(), 0, "Creating");
                    signal.registerSubCircle(subCircle);
                    typedCounter<T>(1);
                }
                ~Via() {
                    halt();
                    typedCounter<T>(-1);
                    signal.unregisterSubCircle(subCircle);
                    LOG_EVENT(typeid(Self).name(), 0, "Destroyed");
                }
                void halt() {
                    if(dying) return;
                    dying = true;
                    signal.notify_all();
                }
                const T value() {
                    return subCircle.popNextValue(&dying);
                }
        };

        template<bool ASYNC, typename T, typename... TARG>
        class GeneralDispatcher : public Via<TARG>... {
            public:
                typedef void(T::*ActionFunction)(TARG...);
                typedef GeneralDispatcher<ASYNC, T, TARG...> Self;
                GeneralDispatcher(const ActionFunction& fn, T* const that_)
                    : Via<TARG>()...
                    , that(that_)
                    , action(fn)
                    , invokations(0)
                    , dying(false)
                {
                    std::unique_lock<std::mutex> l(internal::initGuard);
                    ++internal::initializingDispatchers;
                    firstTick = getCurrentTick();
                    SynchronousDispatcherCounter<ASYNC>::up();
                    LOG_EVENT(typeid(Self).name(), 0, "Created dispatcher on tick " << firstTick);
                    t = std::thread(&Self::run, this);
                }
                ~GeneralDispatcher() {
                    join();
                    SynchronousDispatcherCounter<ASYNC>::down();
                    LOG_EVENT(typeid(Self).name(), 0, "Died");
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
                struct HaltVia {static void halt(Self*){}};
                template<typename T0, typename... TYPES>
                struct HaltVia<T0, TYPES...> {
                    static void halt(Self* obj) {
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
                JiffyType firstTick;
                std::thread t;
                void run() {
                    {
                        LOG_EVENT(typeid(Self).name(), 0, "Synchronizing");
                        std::unique_lock<std::mutex> l(internal::initGuard);
                        while(getCurrentTick() == firstTick) internal::initCondition.wait(l);
                        --internal::initializingDispatchers;
                        if(internal::initializingDispatchers == 0) {
                            internal::allDispatchersInitialized.notify_all();
                        }
                        LOG_EVENT(typeid(Self).name(), 0, "Synchronized");
                    }
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
                    ActiveDispatcherCounter<ASYNC> dc __attribute__((unused));
                    LOG_EVENT(typeid(Self).name(), 0, "Executing actionfunction");
                    (that->*action)(args...);
                    LOG_EVENT(typeid(Self).name(), 0, "Returned from actionfunction.");
                }
        };
    }



    template<typename T, typename... TARG> using Dispatcher = GeneralDispatcher<false, T, TARG...>;
    template<typename T, typename... TARG> using AsynchronousDispatcher = GeneralDispatcher<true, T, TARG...>;
}

#endif
