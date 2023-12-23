#pragma once

#include <string.h>

namespace os {
    template<typename R>
    R fromBytes(const void* data) {
        /// \todo Endianness? */
        R result;
        memcpy((void*)&result, data, sizeof(result));
        return result;
    }


    template<typename T>
    size_t alignmentToAdd(const T* start, const size_t alignment) {
        return (alignment - (size_t)start % alignment) % alignment;
    }

    template<typename T>
    T* addAlignment(const T* start, const size_t alignment) {
        return (T*)((U8*)start + alignmentToAdd(start, alignment));
    }
}
