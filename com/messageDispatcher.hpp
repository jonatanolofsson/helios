#pragma once
#ifndef OS_COM_MESSAGEDISPATCHER_HPP_
#define OS_COM_MESSAGEDISPATCHER_HPP_

#include <os/com/com.hpp>
#include <os/bytemagic.hpp>
#include <iostream>

namespace os {
    template<typename T>
    void messageDispatcher(const U8* data, const std::size_t) {
        //~ static unsigned c = 0;
        //~ std::cout << "Dispatching message " << ++c << " of type " << typeid(T).name() << std::endl;
        yield(fromBytes<T>(data));
    }
}

#endif
