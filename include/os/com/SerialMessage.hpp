#ifndef SERIAL_MESSAGE_HPP_
#define SERIAL_MESSAGE_HPP_

#include <os/types.hpp>

namespace os {

    const std::size_t   SERIAL_MESSAGE_SEPARATOR_LENGTH = 1;
    const U8            SERIAL_MESSAGE_SEPARATOR        = 0xAA;

    struct MessageHeader {
        U16        id;
        U16        length;
        U16        bodyCRC;
        U16        headerCRC;
    };

    const std::size_t  SERIAL_MESSAGE_HEADER_LENGTH    = sizeof(MessageHeader);
    const std::size_t  SERIAL_MESSAGE_ID_OFFSET        = offsetof(MessageHeader, id);
    const std::size_t  SERIAL_MESSAGE_LENGTH_OFFSET    = offsetof(MessageHeader, length);
    const std::size_t  SERIAL_MESSAGE_BODYCRC_OFFSET   = offsetof(MessageHeader, bodyCRC);
    const std::size_t  SERIAL_MESSAGE_HEADERCRC_OFFSET = offsetof(MessageHeader, headerCRC);
}
#endif
