//
// Created by liuzikai on 3/28/21.
//

#ifndef META_VISION_SOLAIS_SERIAL_H
#define META_VISION_SOLAIS_SERIAL_H

#include <string>
#include <cstdint>
#include <boost/asio.hpp>
#include <utility>
#include "FrameCounterBase.h"

namespace meta {

class Serial : public FrameCounterBase {
public:

    explicit Serial(boost::asio::io_context &ioContext);

    enum VisionControlMode : uint8_t {
        RELATIVE_ANGLE = 0,
        ABSOLUTE_ANGLE = 1
    };

    struct __attribute__((packed, aligned(1))) VisionControlCommand {
        uint8_t mode;
        float yaw;
        float pitch;
    };

    bool sendControlCommand(const VisionControlCommand &command);

    using GimbalInfoCallback = std::function<void(float yawAngle, float pitchAngle, float yawVelocity, float pitchVelocity)>;

    void setGimbalInfoCallback(GimbalInfoCallback callback) { gimbalInfoCallback = std::move(callback); }

private:

    // General package header

    static constexpr uint8_t SOF = 0xA5;

    struct __attribute__((packed, aligned(1))) Header {
        uint8_t sof;  // start of header, 0xA5
        uint16_t dataLength;
        uint8_t seq;
        uint8_t crc8;
    };

    // Control command from Vision to Control

    static constexpr uint16_t VISION_CONTROL_CMD_ID = 0xEA01;

    // Gimbal information from Control to Vision

    static constexpr uint16_t GIMBAL_INFO_CMD_ID = 0xEB01;

    struct __attribute__((packed, aligned(1))) GimbalInfo {
        float yawAngle;
        float pitchAngle;
        float yawVelocity;
        float pitchVelocity;
    };

    // Maximal package structure

    struct __attribute__((packed, aligned(1))) Package {
        Header header;
        uint16_t cmdID;
        union {  // union takes the maximal size of its elements
            VisionControlCommand controlCommand;
            GimbalInfo gimbalInfo;
        };
        uint16_t tail;  // just for indication but not use (the offset of this is not correct)
    };

private:

    boost::asio::io_context &ioContext;
    boost::asio::serial_port serial;

    static constexpr int SERIAL_BAUD_RATE = 115200;

    enum ReceiverState {
        RECV_PREAMBLE,          // 0xA5
        RECV_REMAINING_HEADER,  // remaining header after the preamble
        RECV_CMD_ID_DATA_TAIL,  // cmd_id, data section and 2-byte CRC16 tailing
    };

    static constexpr size_t RECV_BUFFER_SIZE = 0x1000;
    ReceiverState recvState = RECV_PREAMBLE;

    uint8_t sendSeq = 0;

    Package recvPackage;

    void handleSend(std::shared_ptr<std::vector<uint8_t>> buf, const boost::system::error_code &error, size_t numBytes);

    void handleRecv(const boost::system::error_code &error, size_t numBytes);

    GimbalInfoCallback gimbalInfoCallback = nullptr;
};
}

#endif //META_VISION_SOLAIS_SERIAL_H
