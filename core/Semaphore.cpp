#include <os/core/Semaphore.hpp>

namespace os {
    void Semaphore::up() {
        std::lock_guard<std::mutex> l(mutex);
        ++value_;
        if (value_ > 0) {
            cond.notify_one();
        }
    }

    void Semaphore::down() {
        std::unique_lock<std::mutex> l(mutex);
        if (value_ <= 0) {
            cond.wait(l, [&] { return value_ > 0; });
        }
        --value_;
    }
    void Semaphore::lower() {
        std::unique_lock<std::mutex> l(mutex);
        --value_;
    }
    int Semaphore::value() {
        std::unique_lock<std::mutex> l(mutex);
        return value_;
    }
}
