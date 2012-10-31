#include <os/types.hpp>
#include <time.h>

namespace os {
    struct clock {
        clock_t system_time;
        clock_t last_time;
        static const int TICKS_PER_SECOND = CLOCKS_PER_SEC;
    };
}
