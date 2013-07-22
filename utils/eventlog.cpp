#include <os/utils/eventlog.hpp>
#include <iostream>
#include <cxxabi.h>

namespace os {
    std::string demangle(const char* name) {
        char buf[1024];
        size_t size=1024;
        int status;
        char* res = abi::__cxa_demangle (name, buf, &size, &status);
        return std::string(res ? res : name);
    }
    std::string demangle(const std::string& name) {
        return name;
    }

    namespace internal {
        std::ostream* eventLogStream = &std::cerr;
        std::mutex eventLogLock;
    }
}
