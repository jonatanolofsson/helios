#pragma once
#ifndef OS_COM_SIGNAL_HPP_
#define OS_COM_SIGNAL_HPP_
#include <mutex>
#include <condition_variable>
#include <os/mem/CircularBuffer.hpp>

namespace os {
    template<typename T>
    class Signal {
        public:
            os::CircularBuffer<T, 10> values;
            ~Signal() { kill(); }
            void kill() { values.kill(); }
            void push(const T& val) { values.push(val); }
            const T nextValue(volatile bool& bailout) { return values.popNextValue(bailout); }
            bool empty() const { return values.empty(); }
            void notify_all() { values.notify_all(); }
    };

    template<typename T> bool getSignal(Signal<T>*& s);

    template<typename T>
    Signal<T>& getSignal() {
        static Signal<T>* signal;
        static bool dummy = getSignal(signal);
        return (dummy ? *signal : *signal); // only call getSignal once, but remove warning for unused dummy var
    }
}
#endif
