#include <os/exceptions.hpp>
#include <os/com/Dispatcher.hpp>

namespace os {
    void running(const int isRunning) {
        static int runningProcesses = 0;
        static std::condition_variable cond;
        static std::mutex guard;
        std::unique_lock<std::mutex> l(guard);

        if(isRunning == 1) {
            ++runningProcesses;
        } else if(isRunning == 0) {
            --runningProcesses;
            if (runningProcesses == 0) {
                cond.notify_all();
            }
        } else {
            while(runningProcesses > 0 && !dying) cond.wait(l);
            if(dying) throw os::HaltException();
        }
    }
}
