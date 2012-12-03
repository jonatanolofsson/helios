#include <stdio.h>
#include <stdarg.h>
#include <os/utils/sprintf.hpp>

namespace os {
    // http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
    std::string sprintf(const std::string &fmt, ...) {
        int size=100;
        std::string str;
        va_list ap;
        while (1) {
            str.resize(size);
            va_start(ap, fmt);
            int n = vsnprintf((char *)str.c_str(), size, fmt.c_str(), ap);
            va_end(ap);
            if (n > -1 && n < size) {
                str.resize(n);
                return str;
            }
            if (n > -1)
                size=n+1;
            else
                size*=2;
        }
    }
}
