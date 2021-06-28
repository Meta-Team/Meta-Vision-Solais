//
// Created by liuzikai on 3/28/21.
//

#include "Serial.h"
#include "CRC.h"
#include <iostream>

namespace meta {

Serial::Serial(boost::asio::io_context &ioContext)
        : ioContext(ioContext), serial(ioContext) {

    boost::system::error_code ec;
    // SERIAL_DEVICE defined in CMakeLists.txt
    serial.open(SERIAL_DEVICE, ec);
    if (ec) {
        std::cerr << "Failed to open serial device \"" << SERIAL_DEVICE << "\": " << ec.message() << std::endl;
        std::exit(1);
    }

    ::tcflush(serial.lowest_layer().native_handle(), TCIOFLUSH);  // flush input and output
    serial.set_option(boost::asio::serial_port::baud_rate(SERIAL_BAUD_RATE));
    serial.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::none));
    serial.set_option(boost::asio::serial_port_base::character_size(8));
    serial.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one));

    // Start receiving SOF
    boost::asio::async_read(serial,
                            boost::asio::buffer(((uint8_t *) &recvPackage), 1),
                            boost::asio::transfer_exactly(1),
                            [this](auto &error, auto numBytes) { handleRecv(error, numBytes); });
}

bool Serial::sendControlCommand(const VisionControlCommand &command) {

    auto buf = std::make_shared<std::vector<uint8_t>>();

    buf->reserve(sizeof(Header) + sizeof(VisionControlCommand) + sizeof(uint16_t));

    // Header
    buf->emplace_back(SOF);
    buf->emplace_back(sizeof(VisionControlCommand) & 0xFF);  // little endian
    buf->emplace_back((sizeof(VisionControlCommand) >> 8) & 0xFF);
    buf->emplace_back(sendSeq++);
    buf->emplace_back(rm::getCRC8CheckSum(buf->data(), sizeof(Header) - sizeof(uint8_t)));

    // Body
    ::memcpy(buf->data() + sizeof(Header), &command, sizeof(VisionControlCommand));

    // Tail
    uint16_t crc16 = rm::getCRC16CheckSum(buf->data(), sizeof(Header) + sizeof(VisionControlCommand));
    buf->emplace_back(crc16 & 0xFF);  // little endian
    buf->emplace_back((crc16 >> 8) & 0xFF);

    boost::asio::async_write(serial,
                             boost::asio::buffer(*buf),
                             [this, buf](auto &error, auto numBytes) { handleSend(buf, error, numBytes); }
    );

    return true;
}

void Serial::handleSend(std::shared_ptr<std::vector<uint8_t>> buf, const boost::system::error_code &error,
                        size_t numBytes) {
    if (error) {
        std::cerr << "Serial: send error: " << error.message() << "\n";
    }
    ++cumulativeFrameCounter;
}

void Serial::handleRecv(const boost::system::error_code &error, size_t numBytes) {

    if (error) {
        std::cerr << "Serial: recv error: " << error.message() << "\n";
        // Continue to process data and start next async_recv
    }

    switch (recvState) {

        case RECV_PREAMBLE:
            if (recvPackage.header.sof == SOF) {
                recvState = RECV_REMAINING_HEADER;
            } // else, keep waiting for SOF
            break;

        case RECV_REMAINING_HEADER:
            if (rm::verifyCRC8CheckSum((uint8_t *) &recvPackage.header, sizeof(Header))) {
                recvState = RECV_CMD_ID_DATA_TAIL; // go to next status
            } else {
                recvState = RECV_PREAMBLE;
            }
            break;

        case RECV_CMD_ID_DATA_TAIL:
            if (rm::verifyCRC16CheckSum(
                    (uint8_t *) &recvPackage,
                    sizeof(Header) + sizeof(uint16_t) + recvPackage.header.dataLength + sizeof(uint16_t))) {

                switch (recvPackage.cmdID) {
                    case GIMBAL_INFO_CMD_ID:
                        if (gimbalInfoCallback) {
                            const auto &info = recvPackage.gimbalInfo;
                            gimbalInfoCallback(info.yawAngle, info.pitchAngle, info.yawVelocity, info.pitchVelocity);
                        }
                        break;
                }
            }

            recvState = RECV_PREAMBLE;
            break;
    }

    size_t offset, size;
    switch (recvState) {
        case RECV_PREAMBLE:
            offset = 0;
            size = sizeof(uint8_t);
            break;
        case RECV_REMAINING_HEADER:
            offset = sizeof(uint8_t);
            size = sizeof(Header) - sizeof(uint8_t);
            break;
        case RECV_CMD_ID_DATA_TAIL:
            offset = sizeof(Header);
            size = sizeof(uint16_t) + recvPackage.header.dataLength + sizeof(uint16_t);
            break;
    }

    boost::asio::async_read(serial,
                            boost::asio::buffer(((uint8_t *) &recvPackage) + offset, size),
                            boost::asio::transfer_exactly(size),
                            [this](auto &error, auto numBytes) { handleRecv(error, numBytes); });
}

}