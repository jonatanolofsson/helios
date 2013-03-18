#pragma once
#ifndef OS_COM_NETWORKCLIENT_HPP_
#define OS_COM_NETWORKCLIENT_HPP_

#include <os/com/NetworkCommunication.hpp>
#include <os/types.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/tcp.h>

namespace os {
    template<U16 PORT, typename M, int MAX_MESSAGE_SIZE, int MAX_QUEUE_LENGTH>
    class NetworkClient : public NetworkCommunication<PORT, M, MAX_MESSAGE_SIZE, MAX_QUEUE_LENGTH> {
        private:
            typedef NetworkCommunication<PORT, M, MAX_MESSAGE_SIZE, MAX_QUEUE_LENGTH> Parent;
            std::string hostName;
            struct hostent *server;

            using Parent::serverAddress;
            using Parent::tcpSocket;
            using Parent::socket;

            void connectToHost() {
                int status;
                do {
                    status = connect(tcpSocket, (const sockaddr *)&serverAddress,sizeof(serverAddress));
                    //~ std::cout << "Would block: " << errno << " == " << EINPROGRESS << std::endl;
                } while(status < 0 && errno == EINPROGRESS);
                if(status < 0 && status ) {
                    std::cerr << "Failed to connect to host: " << status << std::endl;
                    exit(1);
                }
                socket = tcpSocket;
                int flag = 1;
                int err = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
                if(err) {
                    std::cout << "Nodelay failed" << std::endl;
                }            }

            void setupAddress() {
                bcopy((char *)server->h_addr, (char *)&Parent::serverAddress.sin_addr.s_addr, server->h_length);
            }
        public:
            NetworkClient(const std::string hostName_)
            : Parent(hostName_)
            , hostName(hostName_)
            , server(gethostbyname(hostName.c_str()))
            {
                setupAddress();
                Parent::setupSocket();
                connectToHost();
                Parent::start();
            }
    };
}

#endif
