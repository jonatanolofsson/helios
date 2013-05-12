#pragma once
#ifndef OS_COM_SIGNAL_HPP_
#define OS_COM_SIGNAL_HPP_
#include <mutex>
#include <condition_variable>
#include <os/mem/MultiCircularBuffer.hpp>

namespace os {
    static const unsigned SIGNAL_BUFFER_LENGTH      = 10;


    template<typename T>
    class Signal : public os::MultiCircularBuffer<T, SIGNAL_BUFFER_LENGTH> {};

    template<typename T> bool getSignal(Signal<T>*& s);

    template<typename T>
    Signal<T>& getSignal() {
        static Signal<T>* signal;
        static bool dummy = getSignal(signal);
        return (dummy ? *signal : *signal); // only call getSignal once, but remove warning for unused dummy var
    }
}
#endif
