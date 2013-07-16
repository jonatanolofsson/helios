#pragma once
#ifndef OS_COM_SIGNAL_HPP_
#define OS_COM_SIGNAL_HPP_
#include <mutex>
#include <condition_variable>
#include <os/mem/MultiCircularBuffer.hpp>

namespace os {
    static const unsigned SIGNAL_BUFFER_LENGTH      = 10;


    template<typename T>
    using Signal = os::MultiCircularBuffer<T, SIGNAL_BUFFER_LENGTH>;

    template<typename T> Signal<T>& getSignal();
}
#endif
