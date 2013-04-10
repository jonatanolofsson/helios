#pragma once
#ifndef OS_COM_NETWORKSERVER_HPP_
#define OS_COM_NETWORKSERVER_HPP_

#include <os/com/NetworkCommunication.hpp>
#include <os/types.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <thread>

namespace os {
    template<U16 PORT, typename M, int MAX_MESSAGE_SIZE, int MAX_QUEUE_LENGTH>
    class NetworkServer : public NetworkCommunication<PORT, M, MAX_MESSAGE_SIZE, MAX_QUEUE_LENGTH> {
        private:
            typedef NetworkCommunication<PORT, M, MAX_MESSAGE_SIZE, MAX_QUEUE_LENGTH> Parent;
            std::thread acceptThread;
            struct sockaddr_in clientAddress;

            using Parent::serverAddress;
            using Parent::tcpSocket;
            using Parent::socket;
            bool dying;
            bool connected_;

            void setupAddress() {
                serverAddress.sin_addr.s_addr = INADDR_ANY;
            }
            void listenOnConnection() {
                int status = bind(tcpSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
                if(status < 0) {
                    std::cerr << "Failed to bind tcp server socket: " << status << std::endl;
                    exit(1);
                }

                listen(tcpSocket, 5);
            }

            void acceptConnection() {
                int newSocket;
                socklen_t clilen;

                while(!dying) {
                    clilen = sizeof(clientAddress);
                    newSocket = ::accept(tcpSocket, (struct sockaddr *) &clientAddress, &clilen);
                    //~ std::cout << "newSocket == " << newSocket << std::endl;
                    if(newSocket > 0) {
                        //~ std::cout << "Client connected on socket " << newSocket << std::endl;
                        Parent::cleanConnection();
                        socket = newSocket;
                        connected_ = true;
                        int flag = 1;
                        int err = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
                        if(err) {
                            std::cout << "Nodelay failed" << std::endl;
                        }
                        Parent::start();
                    }
                    usleep(1000);
                }
            }

        public:
            NetworkServer()
            : Parent("Server")
            , dying(false)
            , connected_(false)
            {
                setupAddress();
                Parent::setupSocket();
                listenOnConnection();
                acceptThread = std::thread{[this](){this->acceptConnection();}};
            }

            bool connected() {
                return connected_;
            }

            ~NetworkServer() {
                dying = true;
                acceptThread.join();
            }
    };
}

#endif
