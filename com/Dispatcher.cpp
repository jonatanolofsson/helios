#include <os/com/Dispatcher.hpp>
#include <os/com/getSignal.hpp>
#include <os/clock.hpp>

INSTANTIATE_SIGNAL(os::Jiffy);

namespace os {
    void dispatcherActionCounter(const int cmd) {
        static int runningActions = 0;
        static std::mutex guard;
        static Jiffy jiffy;

        bool yieldJiffy;

        {
            std::unique_lock<std::mutex> l(guard);
            //~ std::cout << "Action counter: " << runningActions << " + " << cmd << " = " << runningActions + cmd << std::endl;
            runningActions += cmd;
            yieldJiffy = ((cmd < 0) && (runningActions == 0));
        }

        if (yieldJiffy) {
            yield(++jiffy);
        }
    }
}
