#include <os/com/Dispatcher.hpp>
#include <os/com/getSignal.hpp>
#include <os/clock.hpp>

INSTANTIATE_SIGNAL(os::Jiffy);
INSTANTIATE_SIGNAL(os::SystemTime);

namespace os {
    volatile bool timeIsRunning = false;
    namespace internal {
        std::mutex initGuard;
        std::condition_variable initCondition;
        static Jiffy jiffy;
        int initializingDispatchers = 0;
        std::condition_variable allDispatchersInitialized;

        void activeDispatcherCounter(const int cmd) {
            static int dispatchers = 0;
            static std::mutex guard;

            std::unique_lock<std::mutex> l(guard);
            //LOG_EVENT("activeDispatcherCounter", 0, "Nof dispatchers: " << dispatchers << " + " << cmd << " = " << dispatchers + cmd);
            dispatchers += cmd;

            if ((cmd < 0) && (dispatchers == 0) && timeIsRunning) {
                std::unique_lock<std::mutex> l(internal::initGuard);
                ++internal::jiffy;
                internal::initCondition.notify_all();
                while(internal::initializingDispatchers > 0) internal::allDispatchersInitialized.wait(l);
                dispatchers += SynchronousDispatcherCounter<false>::count();
                getSignal<Jiffy>().push(internal::jiffy);
            }
        }
    }

    void stopTime() {
        timeIsRunning = false;
    }

    void startTime() {
        activeDispatcherCounter(1);
        timeIsRunning = true;
        activeDispatcherCounter(-1);
    }

    JiffyType getCurrentTick() {
        return internal::jiffy.value;
    }

    void expect() {
        activeDispatcherCounter(1);
    }

    void gotExpected() {
        activeDispatcherCounter(-1);
    }
}
