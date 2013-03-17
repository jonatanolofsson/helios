#include <os/exceptions.hpp>
#include <os/com/Dispatcher.hpp>
#include <os/com/getSignal.hpp>
#include <os/clock.hpp>

INSTANTIATE_SIGNAL(os::Jiffy);

namespace os {
    int activeDispatcherActions(DispatcherActivityControl cmd) {
        static int runningActions = 0;
        static bool dying = false;
        static std::condition_variable cond;
        static std::mutex guard;
        static Jiffy jiffy;
        std::unique_lock<std::mutex> l(guard);

        switch(cmd) {
            case DAC_ACTIVATING:
                ++runningActions;
                break;

            case DAC_DEACTIVATING:
                --runningActions;
                if (runningActions == 0) {
                    cond.notify_all();
                    yield(++jiffy);
                }
                break;

            case DAC_WAIT:
                while(runningActions > 0 && !dying) cond.wait(l);
                if(dying) throw os::HaltException();
                break;

            case DAC_DIE:
                dying = true;
                cond.notify_all();
                break;

            case DAC_RESET:
                runningActions = 0;
                dying = false;
                break;

            case DAC_QUERY:
                break;
        }

        return runningActions;
    }
}
