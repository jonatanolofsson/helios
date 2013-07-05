#include <os/utils/eventlog.hpp>
#include <iostream>

namespace os {
    namespace internal {
        std::ostream* eventLogStream = &std::cerr;
        std::mutex eventLogLock;
    }
}
