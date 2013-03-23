#pragma once
#ifndef OS_COM_ASIOCOMMUNICATION_HPP_
#define OS_COM_ASIOCOMMUNICATION_HPP_

#include <os/com/IoCommunication.hpp>
#include <stdint.h>
#include <os/mem/CircularBuffer.hpp>
#include <os/mem/MemoryUnit.hpp>
#include <os/crc.hpp>
#include <os/bytemagic.hpp>

#include <iostream>
#include <exception>
#include <thread>

#include <termios.h>
#include <unistd.h>

namespace os {
    template<typename M, int MAX_MESSAGE_SIZE, int MAX_QUEUE_LENGTH>
    class AsioCommunication : public IoCommunication<M, MAX_MESSAGE_SIZE> {
        private:
            typedef AsioCommunication<M, MAX_MESSAGE_SIZE, MAX_QUEUE_LENGTH> Self;
            typedef IoCommunication<M, MAX_MESSAGE_SIZE> Parent;

        protected:
            using Parent::getName;

            struct Message {
                U8*             body;
                U16             headerCRC;
                U16             bodyCRC;
                MessageHeader   header;
            };

            AsioCommunication(const std::string name_)
            : IoCommunication<M, MAX_MESSAGE_SIZE>(name_)
            , transmitting(false)
            , queue()
            {}

            ~AsioCommunication() {
                close();
            }

            void start() {
                //~ std::cout << "Starting " << getName() << std::endl;
                Parent::start();
                readerThread = std::thread(&Parent::readerLoop, this);
                writerThread = std::thread(&Self::transmitLoop, this);
            }

            typedef typename Parent::MemUnit MemUnit;
            bool transmitting;
            static const int SLEEP_TIME_US = 100;

        private:
            CircularBuffer<MemUnit, MAX_QUEUE_LENGTH> queue;
            std::thread readerThread;
            std::thread writerThread;

            void transmitLoop() {
                //~ std::cout << Parent::getName() << ": " << "Transmitting on " << Parent::socket << std::endl;
                MemUnit* mem;
                while(!Parent::dying) {
                    mem = queue.next();
                    if(mem) {
                        if(Parent::transmit(*mem)) {
                            queue.pop();
                        }
                    }
                }
            }

        public:
            template<typename T>
            void send(const T& contents) {
                static_assert(os::SameType<typename M::template Message<T::ID>::Type, T>::value, "Invalid message type.");
                if(Parent::dying) {
                    return;
                }

                MemUnit* mem = queue.reserve();
                MessageHeader header = { T::ID, sizeof(contents), 0, 0 };
                Parent::signPackage(header, contents);
                mem->cpy(SERIAL_MESSAGE_SEPARATOR);
                mem->template cpy<SERIAL_MESSAGE_SEPARATOR_LENGTH>(header);
                mem->template cpy<sizeof(header)+SERIAL_MESSAGE_SEPARATOR_LENGTH>(contents);
                queue.push();
            }

            template<typename M::Id ID>
            void sendRaw(const U8* contents, const std::size_t len) {
                if(Parent::dying) {
                    return;
                }

                MemUnit* mem = queue.reserve();
                MessageHeader header = { ID, len };
                Parent::signPackage(header, contents);
                mem->cpy(SERIAL_MESSAGE_SEPARATOR);
                mem->cpy<SERIAL_MESSAGE_SEPARATOR_LENGTH>(header);
                mem->cpy<sizeof(header)+SERIAL_MESSAGE_SEPARATOR_LENGTH>(contents, len);
                queue.push();
            }

            void close() {
                if(Parent::dying) return;
                //~ std::cout << getName() << ": Dying" << std::endl;
                Parent::dying = true;
                queue.kill();
                writerThread.join();
                //~ std::cout << getName() << ": Writer died" << std::endl;
                Parent::close();
                readerThread.join();
                //~ std::cout << getName() << ": Reader died" << std::endl;
            }
    };
}

#endif
