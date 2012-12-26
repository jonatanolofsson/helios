#ifndef OS_CRC_HPP_
#define OS_CRC_HPP_

#include <os/types.hpp>

namespace os {
    inline U16 crc16(const U8* const data, const U32 len) {
        U16 crc = 0;
        for(U32 i = 0; i < len; ++i) {
            crc  = (U8)(crc >> 8) | (crc << 8);
            crc ^= data[i];
            crc ^= (U8)(crc & 0xff) >> 4;
            crc ^= (crc << 8) << 4;
            crc ^= ((crc & 0xff) << 4) << 1;
        }
        return crc;
    }
}
#endif
