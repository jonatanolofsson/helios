#pragma once
#ifndef OS_COM_IOCOMMUNICATION_HPP_
#define OS_COM_IOCOMMUNICATION_HPP_

#include <os/crc.hpp>
#include <os/bytemagic.hpp>
#include <os/type_traits.hpp>
#include <os/com/PostOffice.hpp>
#include <os/com/SerialMessage.hpp>
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
#include <thread>
#include <os/exceptions.hpp>
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
                    header.length
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
                //~ #ifndef MAPLE_MINI
                //~ std::cout << getName() << ": " << "Start receive" << std::endl;
                //~ #endif
                reader.reset();
                reader.read(SERIAL_MESSAGE_SEPARATOR_LENGTH, &Self::validateFirstSeparator);
            }

            void invalidMessage() {
                //~ #ifndef MAPLE_MINI
                //~ std::cout << getName() << ": Invalid message" << std::endl;
                //~ #else
                //~ digitalWrite(BOARD_LED_PIN, 1);
                //~ #endif
                startReceive();
            }

            void validateFirstSeparator() {
                //~ #ifndef MAPLE_MINI
                //~ std::cout << getName() << ": " << "Got separator" << std::endl;
                //~ #endif
                if(readMsg[0] == SERIAL_MESSAGE_SEPARATOR) {
                    //~ #ifndef MAPLE_MINI
                    //~ std::cout << getName() << ": " << "Valid separator" << std::endl;
                    //~ #endif
                    reader.reset();
                    reader.read(SERIAL_MESSAGE_SEPARATOR_LENGTH, &Self::validateSecondarySeparators);
                } else {
                    //~ #ifndef MAPLE_MINI
                    //~ std::cout << getName() << ": " << "Invalid separator: " << (int)readMsg[0] << ". " << std::endl;
                    //~ #endif
                    invalidMessage();
                }
            }

            void validateSecondarySeparators() {
                //~ #ifndef MAPLE_MINI
                //~ std::cout << getName() << ": " << "Got next" << std::endl;
                //~ #endif
                if(readMsg[0] == SERIAL_MESSAGE_SEPARATOR) {
                    //~ #ifndef MAPLE_MINI
                    //~ std::cout << getName() << ": " << "Valid separator" << std::endl;
                    //~ #endif
                    // Another separator was received.
                    reader.reset();
                    reader.read(SERIAL_MESSAGE_SEPARATOR_LENGTH, &Self::validateSecondarySeparators);
                } else {
                    //~ #ifndef MAPLE_MINI
                    //~ std::cout << getName() << ": " << "Partial header. Read another " << (SERIAL_MESSAGE_HEADER_LENGTH-1) << " bytes." << std::endl;
                    //~ #endif
                    // No more separators received, interpret as header and continue to read as such
                    reader.read(SERIAL_MESSAGE_HEADER_LENGTH-1, &Self::validateReceivedHeader);
                }
            }

            bool validateHeader() {
                msginfo.header.id        = fromBytes<decltype(msginfo.header.id)>(readMsg        + SERIAL_MESSAGE_ID_OFFSET);
                msginfo.header.length    = fromBytes<decltype(msginfo.header.length)>(readMsg    + SERIAL_MESSAGE_LENGTH_OFFSET);
                msginfo.header.bodyCRC   = fromBytes<decltype(msginfo.header.bodyCRC)>(readMsg   + SERIAL_MESSAGE_BODYCRC_OFFSET);
                msginfo.header.headerCRC = fromBytes<decltype(msginfo.header.headerCRC)>(readMsg + SERIAL_MESSAGE_HEADERCRC_OFFSET);
                //~ std::cout << getName() << ": Id, length, bodyCRC: " << msginfo.header.id << ", " << msginfo.header.length << ", " << msginfo.header.bodyCRC << std::endl;
                //~ U16 shouldBe[4] = {0x10, 0x20, 0x40, 0x80};
                //~ std::cout << getName() << ": Bodycrc should be: " << crc16((U8*)shouldBe, sizeof(shouldBe)) << std::endl;

                msginfo.headerCRC = crc16(
                    readMsg,
                    sizeof(msginfo.header.id) + sizeof(msginfo.header.length) + sizeof(msginfo.header.bodyCRC)
                );
                if(msginfo.headerCRC != msginfo.header.headerCRC) {
                    #ifndef MAPLE_MINI
                    std::cout << getName() << ": Invalid header crc: " << msginfo.headerCRC << " vs " << msginfo.header.headerCRC << std::endl;
                    #endif
                    //~ std::cout << msginfo.header.id << " " << msginfo.header.length << std::endl;
                    return false;
                }
                if(msginfo.header.id >= M::numberOfMessages) {
                    #ifndef MAPLE_MINI
                    std::cout << getName() << ": Invalid id: " << msginfo.header.id << std::endl;
                    #endif
                    return false;
                }

                std::size_t alignment = PostOffice<M>::getAlignment(msginfo.header.id);
                if(alignment == 0) {
                    #ifndef MAPLE_MINI
                    std::cout << getName() << ": Invalid packager: " << msginfo.header.id << std::endl;
                    #endif
                    return false;
                }
                msginfo.body = os::addAlignment(readMsg + reader.offset, alignment);
                reader.offset += os::alignmentToAdd(readMsg + reader.offset, alignment);

                if(sizeof(MessageHeader) + msginfo.header.length + alignment - 1 > MAX_MESSAGE_SIZE) {
                    #ifndef MAPLE_MINI
                    std::cout << getName() << ": Message too big: " << msginfo.header.length << " (+ " << msginfo.header.length << " + " << alignment << " -1 > " << MAX_MESSAGE_SIZE << ")" << std::endl;
                    #endif
                    // Message to big.
                    /// \todo Warn
                    return false;
                }
                return true;
            }

            void validateReceivedHeader() {
                //#ifndef MAPLE_MINI
                //std::cout << "Validating header" << std::endl;
                //#endif
                if(validateHeader()) {
                    //#ifndef MAPLE_MINI
                    //std::cout << getName() << ": " << "Valid header. Receive: " << msginfo.header.length << std::endl;
                    //#endif
                    reader.read(msginfo.header.length, &Self::validateReceivedBody);
                } else {
                    #ifndef MAPLE_MINI
                    std::cout << getName() << ": " << "Invalid header" << std::endl;
                    #endif
                    invalidMessage();
                }
            }

            bool validateBody() {
                msginfo.bodyCRC = crc16(
                    msginfo.body,
                    msginfo.header.length
                );
                //#ifndef MAPLE_MINI
                //std::cout << getName() << ": " << msginfo.header.length << std::endl;
                //std::cout << getName() << ": " << msginfo.bodyCRC << "  =? " << msginfo.header.bodyCRC << std::endl;
                //#endif
                return (msginfo.bodyCRC == msginfo.header.bodyCRC);
            }

            void validateReceivedBody() {
                if(validateBody()) {
                    //#ifndef MAPLE_MINI
                    //std::cout << getName() << ": " << "Valid body" << std::endl;
                    //#endif
                    PostOffice<M>::dispatch(msginfo.header.id, msginfo.body, msginfo.header.length);
                    /* Restart */
                    startReceive();
                } else {
                    #ifndef MAPLE_MINI
                    std::cout << getName() << ": " << "Invalid body" << std::endl;
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
                int retVal = 0;
                if(0 == reader.remaining) return -EINTR;
                reader.receivedBytes  = ::read(socket, &readMsg[reader.offset], reader.remaining);
                retVal = reader.receivedBytes;
                if(reader.receivedBytes  > 0) {
                    reader.remaining -= reader.receivedBytes;
                    reader.offset += reader.receivedBytes;
                    //#ifndef MAPLE_MINI
                    //std::cout << "Read " << reader.receivedBytes << ". Remaining: " << reader.remaining << std::endl;
                    //#endif
                    if(0 == reader.remaining) {
                        if(!dying) {
                            ((this)->*(reader.cb))();
                            return -EAGAIN;
                        }
                    }
                }
                return retVal;
            }

            void readerLoop() {
                //~ std::cout << getName() << ": " << "Reading from " << socket << std::endl;
#ifndef MAPLE_MINI
                try {
#endif
                while(!dying) {
                    #ifndef MAPLE_MINI
                    if(0 == readBytes()) {
                        std::this_thread::yield();
                    }
                    #else
                    readBytes();
                    #endif
                }
#ifndef MAPLE_MINI
                } catch(os::HaltException& e) {}
#endif

                //~ std::cout << getName() << ": Reader died " << socket << std::endl;
                //~ std::cout << getName() << ": Yielded vs noyield: " << yielded << "/" << noyield << " = " << ((float)yielded)/(float)(noyield) << std::endl;
            }

            bool transmit(const MemUnit& mem) {
                bool returnValue = true;
                if(::write(socket, mem.data(), mem.length()) != (int)mem.length())
                {
                    returnValue = false;
                }
                return returnValue;
            }

            #ifndef MAPLE_MINI
            bool sendString(const std::string& mem) {
                bool returnValue = true;
                if(::write(socket, mem.data(), mem.length()) != (int)mem.length())
                {
                    returnValue = false;
                }
                //~ if(!returnValue) {
                    //~ std::cout << "Failed to write string " << mem << " to socket " << socket << std::endl;
                //~ }
                return returnValue;
            }
            #endif

            template<typename T>
            void send(const T& contents, const std::size_t len = sizeof(T)) {
                MemUnit mem;
                if(dying) {
                    return;
                }

                MessageHeader header = { T::ID, len, 0, 0 };
                signPackage(header, contents);

                mem.cpy(0, SERIAL_MESSAGE_SEPARATOR);
                mem.cpy(SERIAL_MESSAGE_SEPARATOR_LENGTH, header);
                mem.cpy(sizeof(header)+SERIAL_MESSAGE_SEPARATOR_LENGTH, contents, len);
                transmit(mem);
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
