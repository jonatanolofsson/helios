#pragma once
#ifndef OS_COM_MESSAGEDISPATCHER_HPP_
#define OS_COM_MESSAGEDISPATCHER_HPP_

#include <os/com/com.hpp>
#include <os/bytemagic.hpp>

namespace os {
    template<typename T>
    void messageDispatcher(const U8* data, const std::size_t) {
        yield(fromBytes<T>(data));
    }
}

#endif
