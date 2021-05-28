//
// Created by liuzikai on 3/28/21.
//

#ifndef META_VISION_SOLAIS_SERIAL_H
#define META_VISION_SOLAIS_SERIAL_H

#include <string>
#include <cstdint>
#include <boost/asio.hpp>
#include "FrameCounterBase.h"

namespace meta {

class Serial : public FrameCounterBase {
public:

    explicit Serial(boost::asio::io_context &ioContext);

    bool sendTargetAngles(float yawDelta, float pitchDelta);

private:

    boost::asio::io_context &ioContext;
    boost::asio::serial_port serial;

    static constexpr int SERIAL_BAUD_RATE = 115200;

    enum ReceiverState {
        RECV_PREAMBLE,  // 0xA5
        RECV_HEADER,    // remaining header after the preamble
        RECV_BODY,      // cmd_id, data section and 2-byte CRC16 tailing
    };

    static constexpr uint8_t PREAMBLE = 0xA5;
    static constexpr size_t RECV_BUFFER_SIZE = 0x10000;

    ReceiverState recvState = RECV_PREAMBLE;

    struct __attribute__((packed, aligned(1))) Header {
        uint8_t sof;  // start of header, 0xA5
        uint16_t dataLength;
        uint8_t seq;
        uint8_t crc8;
    };

    static constexpr uint16_t VISION_CONTROL_CMD_ID = 0xEA01;
    struct __attribute__((packed, aligned(1))) VisionControl {
        float yawDelta;
        float pitchDelta;
    };

    struct __attribute__((packed, aligned(1))) Package {
        Header header;
        uint16_t cmdID;
        union {
            VisionControl vision;
        };
        uint16_t tail;
    };

    uint16_t sendSeq = 0;

    Package recvPackage;

    void handleSend(std::shared_ptr<Package> pkg, const boost::system::error_code &error, size_t numBytes);

    void handleRecv(const boost::system::error_code &error, size_t numBytes);
};
}

#endif //META_VISION_SOLAIS_SERIAL_H
