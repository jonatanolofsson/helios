#include <os/core/Semaphore.hpp>

namespace os {
    void Semaphore::up() {
        std::lock_guard<std::mutex> l(mutex);
        ++value;
        if (value > 0) {
            cond.notify_one();
        }
    }

    void Semaphore::down() {
        std::unique_lock<std::mutex> l(mutex);
        if (value <= 0) {
            cond.wait(l, [&] { return value > 0; });
        }
        --value;
    }
    void Semaphore::lower() {
        std::unique_lock<std::mutex> l(mutex);
        --value;
    }
}
