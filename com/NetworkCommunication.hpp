#pragma once
#ifndef OS_COM_NETWORKCOMMUNICATION_HPP_
#define OS_COM_NETWORKCOMMUNICATION_HPP_

#include <os/com/AsioCommunication.hpp>
#include <os/types.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>

namespace os {
    template<U16 PORT, typename M, int MAX_MESSAGE_SIZE, int MAX_QUEUE_LENGTH>
    class NetworkCommunication : public AsioCommunication<M, MAX_MESSAGE_SIZE, MAX_QUEUE_LENGTH> {
        protected:
            typedef AsioCommunication<M, MAX_MESSAGE_SIZE, MAX_QUEUE_LENGTH> Parent;
            int tcpSocket;
            struct sockaddr_in serverAddress;
            using Parent::getName;

            void setupSocket() {
                tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
                if(tcpSocket < 0) {
                    std::cerr << "Failed to open tcp socket: " << tcpSocket << std::endl;
                    exit(1);
                }

                int error = fcntl(tcpSocket, F_SETFL, O_NONBLOCK);
                if(error < 0) {
                    std::cerr << "Failed to set nonblock: " << tcpSocket << std::endl;
                    exit(1);
                }

                serverAddress.sin_family = AF_INET;
                serverAddress.sin_port = htons(PORT);
            }

            void cleanConnection() {
                if(Parent::socket > 0) {
                    Parent::close();
                }
            }

        public:
            NetworkCommunication(const std::string name = "")
            : Parent(name)
            {
                bzero((char *) &serverAddress, sizeof(serverAddress));
            }

            ~NetworkCommunication() {
                close();
            }

            virtual void close() {
                Parent::close();
                ::close(tcpSocket);
            }
    };
}

#endif
