#ifndef OS_CLOCK_HPP_
#define OS_CLOCK_HPP_

#include <os/types.hpp>
#include <time.h>

namespace os {
    struct clock {
        clock_t system_time;
        clock_t last_time;
        static const int TICKS_PER_SECOND = CLOCKS_PER_SEC;
    };
    struct SystemTime {
        int ticks;
    };
}

#endif
