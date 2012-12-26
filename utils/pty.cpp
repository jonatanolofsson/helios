#include <stdlib.h>
#include <stdio.h>

#include <os/utils/sprintf.hpp>
#include <os/utils/pty.hpp>
#include <signal.h>
#include <unistd.h>

#include <iostream>

#include <exception>

namespace os {
    pty::pty(const std::string& _port1, const std::string& _port2)
    : error(0)
    , port1(_port1)
    , port2(_port2)
    {
        std::string p1 = os::sprintf("pty,raw,echo=0,link=%s", port1.c_str());
        std::string p2 = os::sprintf("pty,raw,echo=0,link=%s", port2.c_str());

        pid = fork();
        if(pid == 0) {
            execl("/usr/bin/socat", "/usr/bin/socat"/*, "-v" */, p1.c_str(), p2.c_str(), NULL);
            error = errno;
            std::cerr << "Socat error : " << errno << std::endl;
        } else if(pid < 0) {
            /// \todo Error handling?
        } else {
            FILE* f = NULL;
            int tries = 0;
            for(; tries < MAX_READ_TRIES; ++tries) {
                f = fopen(port1.c_str(), "r");
                if(f != NULL) {
                    //~ std::cout << "Opened " << port1 << std::endl;
                    fclose(f);
                    break;
                }
                usleep(SLEEP_TIME_US);
            }

            if(tries == MAX_READ_TRIES) {
                /// \todo Error handling?
                throw std::exception();
            }

            for(tries = 0; tries < MAX_READ_TRIES; ++tries) {
                f = fopen(port2.c_str(), "r");
                if(f != NULL) {
                    //~ std::cout << "Opened " << port2 << std::endl;
                    fclose(f);
                    break;
                }
                usleep(SLEEP_TIME_US);
            }

            if(tries == MAX_READ_TRIES) {
                /// \todo Error handling?
                throw std::exception();
            }

            //~ FILE* f2 = NULL;
            //~ U8 buf;
            //~ for(tries = 0; tries < MAX_READ_TRIES; ++tries) {
                //~ usleep(SLEEP_TIME_US);
                //~ f = fopen(port1.c_str(), "r+");
                //~ if(errno) {
                    //~ continue;
                //~ }
                //~ f2 = fopen(port2.c_str(), "r+");
                //~ if(errno) {
                    //~ fclose(f);
                    //~ continue;
                //~ }
//~
                //~ fwrite("a", 1, 1, f);
                //~ if(errno) {
                    //~ fclose(f);
                    //~ fclose(f2);
                    //~ continue;
                //~ } else {
                    //~ fread(&buf, 1, 1, f2);
                    //~ if(errno) {
                        //~ fclose(f);
                        //~ fclose(f2);
                        //~ continue;
                    //~ }
                //~ }
                //~ fwrite("a", 1, 1, f2);
                //~ if(errno) {
                    //~ fclose(f);
                    //~ fclose(f2);
                    //~ continue;
                //~ } else {
                    //~ fread(&buf, 1, 1, f);
                    //~ if(errno) {
                        //~ fclose(f);
                        //~ fclose(f2);
                        //~ continue;
                    //~ }
                //~ }
            //~ }
            //~ std::cout << "Opened tested ptys" << std::endl;
            //~ std::cout << "Opened ptys" << std::endl;
        }
    }

    void pty::close() {
        kill(pid, SIGTERM);
        FILE* f = NULL;
        int tries;
        //~ std::cout << "Closing " << port1 << std::endl;
        for(tries = 0; tries < MAX_READ_TRIES; ++tries) {
            f = fopen(port1.c_str(), "r");
            if(f != NULL) {
                fclose(f);
            } else {
                break;
            }
            usleep(SLEEP_TIME_US);
        }
        //~ std::cout << "Closing " << port2 << std::endl;
        for(tries = 0; tries < MAX_READ_TRIES; ++tries) {
            f = fopen(port2.c_str(), "r");
            if(f != NULL) {
                fclose(f);
            } else {
                break;
            }
            usleep(SLEEP_TIME_US);
        }
    }
    pty::~pty() {
        close();
    }
}
