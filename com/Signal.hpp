#pragma once
#ifndef OS_COM_SIGNAL_HPP_
#define OS_COM_SIGNAL_HPP_
#include <mutex>
#include <condition_variable>

namespace os {
    template<typename T>
    class Signal {
        public:
            T value;
            std::mutex guard;
            std::condition_variable cond;
            int id;
            Signal() : id(0) {}
            void operator=(const T& val) {
                std::lock_guard<std::mutex> l(guard);
                value = val;
                ++id;
                cond.notify_all();
            }
            const T operator*() {
                return value;
            }
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
