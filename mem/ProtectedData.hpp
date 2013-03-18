#pragma once
#ifndef OS_SHARED_PROTECTED_DATA_HPP_
#define OS_SHARED_PROTECTED_DATA_HPP_

#include <mutex>

namespace os {
    class Nothing {};
    template<typename T>
    class ProtectedData : public T {
        private:
            std::mutex guard;
            std::unique_lock<std::mutex> l;

            void lock() {
                l.lock();
            }
            void unlock() {
                l.unlock();
            }

        public:
            ProtectedData() : l(guard, std::defer_lock) {}
            std::unique_lock<std::mutex> retrieve_lock() const {
                return std::unique_lock<std::mutex>(const_cast<std::mutex&>(guard));
            }
    };
    typedef ProtectedData<Nothing> ProtectedClass;
}
#endif
