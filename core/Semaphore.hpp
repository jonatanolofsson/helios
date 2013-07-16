#pragma once
#ifndef OS_CORE_SEMAPHORE_HPP_
#define OS_CORE_SEMAPHORE_HPP_

#include <mutex>
#include <condition_variable>

namespace os {
    class Semaphore {
        private:
            int value;
            std::mutex mutex;
            std::condition_variable cond;

        public:
            Semaphore(int value = 0) : value(value) {}

            void up();
            void down();
            void lower();
    };

    class SemaphoreGuard {
        private:
            SemaphoreGuard();
            Semaphore& sem;
        public:
            SemaphoreGuard(Semaphore& s) : sem(s) {
                sem.lower();
            }
            ~SemaphoreGuard() {
                sem.up();
            }
    };
}

#endif

