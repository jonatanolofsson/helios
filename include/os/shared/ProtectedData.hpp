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

        public:
            ProtectedData() : l(guard, std::defer_lock) {}
            void lock() {
                l.lock();
            }
            void unlock() {
                l.unlock();
            }
    };
    typedef ProtectedData<Nothing> ProtectedClass;
}
#endif
