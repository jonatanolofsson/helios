#pragma once
#ifndef OS_CORE_SEMAPHORE_HPP_
#define OS_CORE_SEMAPHORE_HPP_

#include <mutex>
#include <condition_variable>
#include <os/utils/eventlog.hpp>

namespace os {
    class Semaphore {
        private:
            int value_;
            std::mutex mutex;
            std::condition_variable cond;

        public:
            Semaphore(int value = 0) : value_(value) {}

            void up();
            void down();
            void lower();
            int value();
    };

    class SemaphoreGuard {
        public:
            typedef SemaphoreGuard Self;
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

