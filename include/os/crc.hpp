#ifndef OS_CRC_HPP_
#define OS_CRC_HPP_

#include <boost/crc.hpp>
#include <os/types.hpp>

namespace os {
    inline U32 crc32(const U8* data, const U32 len) {
        boost::crc_32_type result;
        result.process_bytes(data, len);
        return result.checksum();
    }
    inline U16 crc16(const U8* data, const U32 len) {
        boost::crc_16_type result;
        result.process_bytes(data, len);
        return result.checksum();
    }
}
#endif
