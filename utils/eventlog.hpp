#pragma once
#ifndef OS_UTILS_EVENTLOG_HPP_
#define OS_UTILS_EVENTLOG_HPP_

#include <mutex>
#include <iostream>

#define LOG_EVENT(id, lvl, e)    do { std::unique_lock<std::mutex> _eventLogLock__(os::internal::eventLogLock); *os::internal::eventLogStream << os::demangle(id) << " [" << lvl << "] : " << e << std::endl; } while(0);


namespace os {
    std::string demangle(const char* name);

    namespace internal {
        extern std::ostream* eventLogStream;
        extern std::mutex eventLogLock;
    }
}

#endif
