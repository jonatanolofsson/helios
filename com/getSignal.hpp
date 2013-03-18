#pragma once
#ifndef OS_COM_GETSIGNAL_HPP_
#define OS_COM_GETSIGNAL_HPP_

#include <os/com/Signal.hpp>

#define INSTANTIATE_SIGNAL(type)    template bool os::getSignal<type>(os::Signal<type>*&)

namespace os {
    template<typename T> bool getSignal(Signal<T>*& s) {
        static Signal<T> signal;
        s = &signal;
        return true;
    }
}
#endif
