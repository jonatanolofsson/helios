#include <os/exceptions.hpp>
#include <os/com/Dispatcher.hpp>
#include <os/com/getSignal.hpp>
#include <os/clock.hpp>

INSTANTIATE_SIGNAL(os::Jiffy);

namespace os {
    void dispatcherActionCounter(const int cmd) {
        static int runningActions = 0;
        static std::condition_variable cond;
        static std::mutex guard;
        static Jiffy jiffy;
        std::unique_lock<std::mutex> l(guard);

        runningActions += (int)cmd;
        if (cmd < 0 && runningActions == 0) {
            cond.notify_all();
            yield(++jiffy);
        }
    }
}
