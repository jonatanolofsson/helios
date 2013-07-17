#pragma once
#ifndef OS_COM_GETSIGNAL_HPP_
#define OS_COM_GETSIGNAL_HPP_

#include <os/com/Signal.hpp>
#define SIGNALNAME_MERGE2(a,b) a##b
#define SIGNALNAME_MERGE(a,b) SIGNALNAME_MERGE2(a,b)
#define INSTANTIATE_SIGNAL(T) template os::Signal<T>& os::getSignal<T>(); os::InitializeSignal<T> SIGNALNAME_MERGE(__tmp, __COUNTER__)()

namespace os {
    template<typename T> struct InitializeSignal {
        typedef InitializeSignal<T> Self;
        InitializeSignal() {
            os::getSignal<T>();
        }
    };
    template<typename T> Signal<T>& getSignal() {
        static Signal<T> signal;
        return signal;
    }
}
#endif
