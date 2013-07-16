#pragma once
#ifndef OS_CLOCK_HPP_
#define OS_CLOCK_HPP_

#include <os/types.hpp>
#include <time.h>
#include <iostream>

namespace os {
    struct clock {
        clock_t system_time;
        clock_t last_time;
        static const int TICKS_PER_SECOND = CLOCKS_PER_SEC;
    };

    typedef U64 TimeType;
    struct SystemTime {
        TimeType value;
        SystemTime() : value(0) {}
        explicit SystemTime(const int v) : value(v) {}
        SystemTime& operator++() { ++value; return *this; }
    };
    inline std::ostream& operator<<(std::ostream& os, const SystemTime& t) { return os << t.value; }

    typedef U64 JiffyType;
    struct Jiffy {
        JiffyType value;
        Jiffy() : value(0) {}
        explicit Jiffy(const int v) : value(v) {}
        Jiffy& operator++() { ++value; return *this; }
    };
    inline std::ostream& operator<<(std::ostream& os, const Jiffy& j) { return os << j.value; }
}

#endif
