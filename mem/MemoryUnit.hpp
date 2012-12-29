#ifndef OS_MEM_MEMORYUNIT_HPP_
#define OS_MEM_MEMORYUNIT_HPP_

#include <os/bytemagic.hpp>

namespace os {
    template<std::size_t SIZE, std::size_t ALIGN = 1>
    class MemoryUnit {
        private:
            U64 length_;
            U8 storage[SIZE + ALIGN - 1];
            U8* data_nc() const {
                return os::addAlignment(storage, ALIGN);
            }

        public:
            MemoryUnit() : length_(0) {}
            const U8* data() const {
                return data_nc();
            }
            std::size_t length() const {
                return length_;
            }
            template<unsigned OFFSET = 0, typename T>
            void cpy(const T& d, const std::size_t len = sizeof(T)) {
                if((OFFSET + len) <= SIZE) {
                    memcpy(data_nc() + OFFSET, &d, len);
                    length_ = (length_ > (OFFSET + len) ? length_ : (OFFSET + len));
                } else {
                    //~ std::cerr << "Tried to overfill membuffer" << std::endl;
                }
            }
    };
}

#endif
