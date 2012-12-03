#ifndef OS_COM_ASIOCOMMUNICATION_HPP_
#define OS_COM_ASIOCOMMUNICATION_HPP_

#include <stdint.h>
#include <os/mem/CircularBuffer.hpp>
#include <os/mem/MemoryUnit.hpp>
#include <os/crc.hpp>
#include <os/bytemagic.hpp>

#include <iostream>
#include <exception>
#include <thread>
#include <boost/thread.hpp>

#include <termios.h>
#include <unistd.h>

namespace os {
    template<typename T, typename M, int MAX_MESSAGE_SIZE, int MAX_QUEUE_LENGTH>
    class AsioCommunication : public PostOffice<M> {
        protected:
            struct Message {
                U8*             body;
                U16             headerCRC;
                U16             bodyCRC;
                MessageHeader   header;
            };

            AsioCommunication(const std::string name_)
            : PostOffice<M>()
            , socket(0)
            , dying(false)
            , transmitting(false)
            , name(name_)
            , queue()
            {
                //~ std::cout << getName() << ": " << "Construct " << std::endl;
            }

            void start() {
                //~ std::cout << getName() << ": " << "Start: " << socket << std::endl;
                assert(socket > 0);
                startReceive();
                readerThread = std::thread(&Self::readerLoop, this);
                //~ std::cout << getName() << ": " << "Start 2: " << socket << std::endl;
                writerThread = std::thread(&Self::transmitLoop, this);
            }

            ~AsioCommunication() {
                close();
            }

            typedef MemoryUnit<MAX_MESSAGE_SIZE, alignof(MessageHeader)> MemUnit;
            int socket;
            bool dying;
            bool transmitting;
            std::string name;
            static const int SLEEP_TIME_US = 100;

        private:
            typedef AsioCommunication<T, M, MAX_MESSAGE_SIZE, MAX_QUEUE_LENGTH> Self;
            U8 readMsg[MAX_MESSAGE_SIZE];
            CircularBuffer<MemUnit, MAX_QUEUE_LENGTH> queue;
            Message msginfo;
            std::thread readerThread;
            std::thread writerThread;

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

            void startReceive() {
                //~ std::cout << getName() << ": " << "Start receive" << std::endl;
                reader.reset();
                reader.read(SERIAL_MESSAGE_SEPARATOR_LENGTH, &Self::validateFirstSeparator);
            }

            void validateFirstSeparator() {
                //~ std::cout << getName() << ": " << "Got separator, #" << std::endl;
                if((readMsg[0] == SERIAL_MESSAGE_SEPARATOR)) {
                    //~ std::cout << getName() << ": " << "Valid separator" << std::endl;
                    reader.reset();
                    reader.read(SERIAL_MESSAGE_SEPARATOR_LENGTH, &Self::validateSecondarySeparators);
                } else {
                    //~ std::cerr << getName() << ": " << "Invalid separator. " << std::endl;
                    startReceive();
                }
            }

            void validateSecondarySeparators() {
                if(readMsg[0] == SERIAL_MESSAGE_SEPARATOR) {
                    //~ std::cout << getName() << ": " << "Valid separator" << std::endl;
                    // Another separator was received.
                    reader.reset();
                    reader.read(SERIAL_MESSAGE_SEPARATOR_LENGTH, &Self::validateSecondarySeparators);
                } else {
                    //~ std::cout << getName() << ": " << "Partial header" << std::endl;
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
                    std::cerr << getName() << ": Invalid header crc: " << msginfo.headerCRC << " vs " << msginfo.header.headerCRC << std::endl;
                    std::cerr << msginfo.header.id << " " << msginfo.header.length << std::endl;
                    return false;
                }

                std::size_t alignment = PostOffice<M>::getAlignment(msginfo.header.id);
                msginfo.body = os::addAlignment(readMsg + reader.offset, alignment);
                reader.offset += os::alignmentToAdd(readMsg + reader.offset, alignment);

                if(sizeof(MessageHeader) + msginfo.header.length + alignment - 1 > MAX_MESSAGE_SIZE) {
                    std::cerr << getName() << ": Message too big: " << msginfo.header.length << std::endl;
                    // Message to big.
                    /// \todo Warn
                    return false;
                }
                return true;
            }

            void validateReceivedHeader() {
                if(validateHeader()) {
                    //~ std::cout << getName() << ": " << "Valid header. Receive: " << msginfo.header.length << std::endl;
                    reader.read(msginfo.header.length, &Self::validateReceivedBody);
                } else {
                    std::cerr << getName() << ": " << "Invalid header" << std::endl;
                }
            }

            bool validateBody() {
                msginfo.bodyCRC = crc16(
                    msginfo.body,
                    msginfo.header.length
                );
                return (msginfo.bodyCRC == msginfo.header.bodyCRC);
            }

            void validateReceivedBody() {
                if(validateBody()) {
                    //~ std::cout << getName() << ": " << "Valid body" << std::endl;
                    PostOffice<M>::dispatch(msginfo.header.id, msginfo.body, msginfo.header.length);
                } else {
                    std::cerr << getName() << ": " << "Invalid body" << std::endl;
                }

                /* Restart */
                startReceive();
            }

            struct reader_ {
                typedef void(Self::*Callback)();
                size_t length;
                size_t offset;
                Callback cb;

                void reset() {
                    offset = 0;
                }

                void read(unsigned int length_, Callback cb_) {
                    length = length_;
                    cb = cb_;
                }
            } reader;

            void readerLoop() {
                //~ std::cout << getName() << ": " << "Reading from " << socket << std::endl;
                while(!dying) {
                    int serial_recv_byte_count = 0;
                    size_t remaining = reader.length;
                    while(remaining > 0 && !dying) {
                        if (( serial_recv_byte_count  = ::read(socket, &readMsg[reader.offset], remaining)) < 0 ) {
                            //~ std::cerr << "read() failed: " << errno << std::endl;
                            //~ usleep(SLEEP_TIME_US);
                            //~ startReceive();
                            remaining = reader.length;
                        } else {
                            //~ if(serial_recv_byte_count>0) std::cout << "Read " << serial_recv_byte_count << std::endl;
                            remaining -= serial_recv_byte_count;
                            reader.offset += serial_recv_byte_count;
                        }
                    }
                    if(!dying) {
                        ((this)->*(reader.cb))();
                    }
                }
            }

            void transmitLoop() {
                //~ std::cout << "Transmitting on " << socket << std::endl;
                MemUnit* mem;
                while(!dying) {
                    mem = queue.next();
                    if(mem) {
                        if(::write(socket, &SERIAL_MESSAGE_SEPARATOR, SERIAL_MESSAGE_SEPARATOR_LENGTH) != (int)SERIAL_MESSAGE_SEPARATOR_LENGTH) {
                            std::cerr << "write(" << socket << ") failed: " << errno << std::endl;
                        }
                        else if(::write(socket, mem->data(), mem->length()) != (int)mem->length()) {
                            std::cerr << "write(" << socket << ") failed: " << errno << std::endl;
                        }
                        else {
                            queue.pop();
                        }
                    }
                }
            }

        public:
            template<typename M::Id ID>
            void send(const typename M::template Message<ID>::Type& contents) {
                if(dying) {
                    return;
                }

                MemUnit* mem = queue.reserve();
                MessageHeader header = { ID, sizeof(contents), 0, 0 };
                signPackage(header, contents);
                mem->cpy(header);
                mem->template cpy<sizeof(header)>(contents);
                queue.push();
            }

            template<typename M::Id ID>
            void sendRaw(const U8* contents, const std::size_t len) {
                if(dying) {
                    return;
                }

                MemUnit* mem = queue.reserve();
                MessageHeader header = { ID, len };
                signPackage(header, contents);
                mem->cpy(&header);
                mem->cpy<sizeof(header)>(&contents, len);
                queue.push();
            }

            std::string getName() const {
                return name;
            }

            void close() {
                //~ std::cout << "Dying" << std::endl;
                dying = true;
                queue.kill();
                writerThread.join();
                //~ std::cout << "Writer died" << std::endl;
                ::close(socket);
                socket = 0;
                readerThread.join();
                //~ std::cout << "Reader died" << std::endl;
            }
    };
}

#endif
