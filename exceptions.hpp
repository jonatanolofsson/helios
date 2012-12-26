#ifndef OS_EXCEPTIONS_HPP_
#define OS_EXCEPTIONS_HPP_

#include <exception>

namespace os {
    struct HaltException : public std::exception {};
}

#endif
