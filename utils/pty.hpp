#pragma once
#ifndef OS_UTILS_OPENPTY_HPP_
#define OS_UTILS_OPENPTY_HPP_

#include <sys/types.h>
#include <string>

namespace os {
    class pty {
        private:
            int error;
            pid_t pid;
            static const int MAX_READ_TRIES = 100;
            static const int SLEEP_TIME_US  = 1e3;

            std::string port1;
            std::string port2;

        public:
            pty(const std::string& port1, const std::string& port2);
            void close();
            ~pty();
    };
}

#endif
