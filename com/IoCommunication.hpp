#pragma once
#ifndef OS_COM_IOCOMMUNICATION_HPP_
#define OS_COM_IOCOMMUNICATION_HPP_

#include <os/crc.hpp>
#include <os/bytemagic.hpp>
#include <os/type_traits.hpp>
#include <os/com/PostOffice.hpp>
#include <os/mem/MemoryUnit.hpp>
#include <errno.h>

#ifdef MAPLE_MINI
#include <wirish/wirish.h>
#include <syrup/comm/ByteInterface.hpp>
using namespace syrup;
#else
#include <unistd.h>
#include <string>
#include <iostream>
#endif

namespace os {
    template<typename M, int MAX_MESSAGE_SIZE>
    class IoCommunication : public PostOffice<M> {
        private:
            typedef IoCommunication<M, MAX_MESSAGE_SIZE> Self;
            typedef PostOffice<M> Parent;

        protected:
            struct Message {
                U8*             body;
                U16             headerCRC;
                U16             bodyCRC;
                MessageHeader   header;
            };

            explicit IoCommunication(
#ifndef MAPLE_MINI
            const std::string name_
#endif
            )
            : Parent()
            , socket(0)
            , dying(false)
#ifndef MAPLE_MINI
            , name(name_)
#endif
            {}

            void start() {
                dying = false;
                //~ assert(socket > 0);
                startReceive();
            }

            ~IoCommunication() {
                close();
            }

            typedef MemoryUnit<MAX_MESSAGE_SIZE, alignof(MessageHeader)> MemUnit;
            int socket;
            bool dying;
#ifndef MAPLE_MINI
            std::string name;
#endif
            static const int SLEEP_TIME_US = 100;

            template<typename TT>
            void signPackage(MessageHeader& header, const TT& contents) {
                header.bodyCRC = crc16(
                    (U8*)&contents,
                    sizeof(contents)
                );
                header.headerCRC = crc16(
                    (U8*)&header,
                    sizeof(header.id) + sizeof(header.length) + sizeof(header.bodyCRC)
                );
            }

        private:
            U8 readMsg[MAX_MESSAGE_SIZE];
            Message msginfo;

            void startReceive() {
                #ifndef MAPLE_MINI
                //~ std::cout << getName() << ": " << "Start receive" << std::endl;
                #endif
                reader.reset();
                reader.read(SERIAL_MESSAGE_SEPARATOR_LENGTH, &Self::validateFirstSeparator);
            }

            void invalidMessage() {
                #ifndef MAPLE_MINI
                //~ std::cout << "Invalid message" << std::endl;
                #else
                //~ digitalWrite(BOARD_LED_PIN, 1);
                #endif
                startReceive();
            }

            void validateFirstSeparator() {
                #ifndef MAPLE_MINI
                //~ std::cout << getName() << ": " << "Got separator, #" << std::endl;
                #endif
                if((readMsg[0] == SERIAL_MESSAGE_SEPARATOR)) {
                    //~ std::cout << getName() << ": " << "Valid separator" << std::endl;
                    reader.reset();
                    reader.read(SERIAL_MESSAGE_SEPARATOR_LENGTH, &Self::validateSecondarySeparators);
                } else {
                    #ifndef MAPLE_MINI
                    //~ std::cerr << getName() << ": " << "Invalid separator: " << (int)readMsg[0] << ". " << std::endl;
                    #endif
                    invalidMessage();
                }
            }

            void validateSecondarySeparators() {
                #ifndef MAPLE_MINI
                //~ std::cout << getName() << ": " << "Got next" << std::endl;
                #endif
                if(readMsg[0] == SERIAL_MESSAGE_SEPARATOR) {
                    #ifndef MAPLE_MINI
                    //~ std::cout << getName() << ": " << "Valid separator" << std::endl;
                    #endif
                    // Another separator was received.
                    reader.reset();
                    reader.read(SERIAL_MESSAGE_SEPARATOR_LENGTH, &Self::validateSecondarySeparators);
                } else {
                    #ifndef MAPLE_MINI
                    //~ std::cout << getName() << ": " << "Partial header. Read another " << (SERIAL_MESSAGE_HEADER_LENGTH-1) << " bytes." << std::endl;
                    #endif
                    // No more separators received, interpret as header and continue to read as such
                    reader.read(SERIAL_MESSAGE_HEADER_LENGTH-1, &Self::validateReceivedHeader);
                }
            }

            bool validateHeader() {
                msginfo.header.id        = fromBytes<decltype(msginfo.header.id)>(readMsg        + SERIAL_MESSAGE_ID_OFFSET);
                msginfo.header.length    = fromBytes<decltype(msginfo.header.length)>(readMsg    + SERIAL_MESSAGE_LENGTH_OFFSET);
                msginfo.header.bodyCRC   = fromBytes<decltype(msginfo.header.bodyCRC)>(readMsg   + SERIAL_MESSAGE_BODYCRC_OFFSET);
                msginfo.header.headerCRC = fromBytes<decltype(msginfo.header.headerCRC)>(readMsg + SERIAL_MESSAGE_HEADERCRC_OFFSET);

                msginfo.headerCRC = crc16(
                    readMsg,
                    sizeof(MessageHeader::id) + sizeof(MessageHeader::length) + sizeof(MessageHeader::bodyCRC)
                );
                if(msginfo.headerCRC != msginfo.header.headerCRC) {
                    #ifndef MAPLE_MINI
                    //~ std::cerr << getName() << ": Invalid header crc: " << msginfo.headerCRC << " vs " << msginfo.header.headerCRC << std::endl;
                    #endif
                    //~ std::cerr << msginfo.header.id << " " << msginfo.header.length << std::endl;
                    return false;
                }
                if(msginfo.header.id >= M::numberOfMessages) {
                #ifndef MAPLE_MINI
                    //~ std::cerr << getName() << ": Invalid id: " << msginfo.header.id << std::endl;
                #endif
                    return false;
                }

                std::size_t alignment = PostOffice<M>::getAlignment(msginfo.header.id);
                if(alignment == 0) {
                #ifndef MAPLE_MINI
                    //~ std::cerr << getName() << ": Invalid packager: " << msginfo.header.id << std::endl;
                #endif
                    return false;
                }
                msginfo.body = os::addAlignment(readMsg + reader.offset, alignment);
                reader.offset += os::alignmentToAdd(readMsg + reader.offset, alignment);

                if(sizeof(MessageHeader) + msginfo.header.length + alignment - 1 > MAX_MESSAGE_SIZE) {
                #ifndef MAPLE_MINI
                    //~ std::cerr << getName() << ": Message too big: " << msginfo.header.length << std::endl;
                #endif
                    // Message to big.
                    /// \todo Warn
                    return false;
                }
                return true;
            }

            void validateReceivedHeader() {
                if(validateHeader()) {
                    #ifndef MAPLE_MINI
                    //~ std::cout << getName() << ": " << "Valid header. Receive: " << msginfo.header.length << std::endl;
                    #endif
                    reader.read(msginfo.header.length, &Self::validateReceivedBody);
                } else {
                    #ifndef MAPLE_MINI
                    std::cerr << getName() << ": " << "Invalid header" << std::endl;
                    #endif
                    invalidMessage();
                }
            }

            bool validateBody() {
                msginfo.bodyCRC = crc16(
                    msginfo.body,
                    msginfo.header.length
                );
                #ifndef MAPLE_MINI
                //~ std::cerr << getName() << ": " << msginfo.header.length << std::endl;
                //~ std::cerr << getName() << ": " << msginfo.bodyCRC << "  =? " << msginfo.header.bodyCRC << std::endl;
                #endif
                return (msginfo.bodyCRC == msginfo.header.bodyCRC);
            }

            void validateReceivedBody() {
                if(validateBody()) {
                    #ifndef MAPLE_MINI
                    //~ std::cout << getName() << ": " << "Valid body" << std::endl;
                    #endif
                    PostOffice<M>::dispatch(msginfo.header.id, msginfo.body, msginfo.header.length);
                    /* Restart */
                    startReceive();
                } else {
                    #ifndef MAPLE_MINI
                    std::cerr << getName() << ": " << "Invalid body" << std::endl;
                    #endif
                    invalidMessage();
                }
            }

            struct reader_ {
                typedef void(Self::*Callback)();
                int length;
                int offset;
                int remaining;
                int receivedBytes;
                Callback cb;

                void reset() {
                    offset = 0;
                }

                void read(unsigned int length_, Callback cb_) {
                    length = length_;
                    cb = cb_;
                    remaining = length;
                }
            } reader;

        public:
            int readBytes() {
                if(0 == reader.remaining) return -EINTR;
                reader.receivedBytes  = ::read(socket, &readMsg[reader.offset], reader.remaining);
                if(reader.receivedBytes  > 0) {
                    reader.remaining -= reader.receivedBytes;
                    reader.offset += reader.receivedBytes;
                    #ifndef MAPLE_MINI
                    //~ if(receivedBytes>0) std::cout << "Read " << receivedBytes << ". Remaining: " << remaining << std::endl;
                    #endif
                    if(0 == reader.remaining) {
                        if(!dying) {
                            ((this)->*(reader.cb))();
                            return -EAGAIN;
                        }
                    }
                }
                return 0;
            }

            void readerLoop() {
                //~ std::cout << getName() << ": " << "Reading from " << socket << std::endl;
                while(!dying) {
                    readBytes();
                }
                //~ std::cout << getName() << ": Reader died " << socket << std::endl;
            }

            bool transmit(const MemUnit& mem) {
                bool returnValue = true;
                if(::write(socket, mem.data(), mem.length()) != (int)mem.length())
                {
                    returnValue = false;
                }
                return returnValue;
            }


            MemUnit mem[2];

            template<typename T>
            void send(const T& contents) {
                if(dying) {
                    return;
                }

                static bool n = 0;
                MessageHeader header = { T::ID, sizeof(contents), 0, 0 };
                signPackage(header, contents);

                mem[n].cpy(SERIAL_MESSAGE_SEPARATOR);
                mem[n].template cpy<SERIAL_MESSAGE_SEPARATOR_LENGTH>(header);
                mem[n].template cpy<sizeof(header)+SERIAL_MESSAGE_SEPARATOR_LENGTH>(contents);
                transmit(mem[n]);
                n = !n;
            }
#ifndef MAPLE_MINI
            std::string getName() const {
                return name;
            }
#endif

            virtual void close() {
                dying = true;
                if(socket) {
                    ::close(socket);
                }
                socket = 0;
            }
    };
}

#endif
