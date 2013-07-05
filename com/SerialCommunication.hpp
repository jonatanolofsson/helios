#pragma once
#ifndef OS_COM_SERIALCOMMUNICATION_HPP_
#define OS_COM_SERIALCOMMUNICATION_HPP_

#include <os/com/AsioCommunication.hpp>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

namespace os {
    template<typename M, int MAX_MESSAGE_SIZE, int MAX_QUEUE_LENGTH, int BAUD_RATE = B115200>
    class SerialCommunication : public AsioCommunication<M, MAX_MESSAGE_SIZE, MAX_QUEUE_LENGTH> {
        public:
            typedef M Messages;
            typedef SerialCommunication<M, MAX_MESSAGE_SIZE, MAX_QUEUE_LENGTH, BAUD_RATE> Self;

        private:
            typedef AsioCommunication<M, MAX_MESSAGE_SIZE, MAX_QUEUE_LENGTH> Parent;
            std::string portName;
            struct termios config;

        public:
            SerialCommunication(const std::string& portName_)
            : Parent(portName_)
            , portName(portName_)
            {
                int tries = 0;
                while(Parent::socket == 0 && ++tries <= 10) {
                    Parent::socket = ::open(portName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK /*| O_NDELAY*/);//
                }
                //~ std::cout << "Opened " << portName << ": " << Parent::socket << "; errno = " << errno << std::endl;

                if(BAUD_RATE) {
                    memset(&config, 0, sizeof(config));
                    // Input flags - Turn off input processing
                    // convert break to null byte, no CR to NL translation,
                    // no NL to CR translation, don't mark parity errors or breaks
                    // no input parity check, don't strip high bit off,
                    // no XON/XOFF software flow control
                    //
                    config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
                                        INLCR | PARMRK | INPCK | ISTRIP | IXON);
                    //
                    // Output flags - Turn off output processing
                    // no CR to NL translation, no NL to CR-NL translation,
                    // no NL to CR translation, no column 0 CR suppression,
                    // no Ctrl-D suppression, no fill characters, no case mapping,
                    // no local output processing
                    //
                    // config.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
                    //                     ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
                    config.c_oflag = 0;

                    //
                    // No line processing:
                    // echo off, echo newline off, canonical mode off,
                    // extended input processing off, signal chars off
                    //
                    config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

                    //
                    // Turn off character processing
                    // clear current char size mask, no parity checking,
                    // no output processing, force 8 bit input
                    //
                    config.c_cflag &= ~(CSIZE | PARENB);
                    config.c_cflag |= CS8;
                    //
                    // One input byte is enough to return from read()
                    // Inter-character timer off
                    //
                    config.c_cc[VMIN]  = 1;
                    config.c_cc[VTIME] = 0;

                    if(cfsetispeed(&config, BAUD_RATE) < 0 || cfsetospeed(&config, BAUD_RATE) < 0) {
                        std::cout << "Failed to set baud rate" << std::endl;
                    }
                    if(tcsetattr(Parent::socket, TCSAFLUSH, &config) < 0) {
                        std::cout << "Failed to flush settings" << std::endl;
                    }
                }

                Parent::start();
            }
    };
}

#endif
