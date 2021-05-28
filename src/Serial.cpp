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
}

bool Serial::sendTargetAngles(float yawDelta, float pitchDelta) {

    auto pkg = std::make_shared<Package>();

    pkg->header.sof = PREAMBLE;
    pkg->header.dataLength = sizeof(pkg->vision);
    pkg->header.seq = sendSeq++;
    rm::appendCRC8CheckSum((uint8_t *) &pkg->header, sizeof(pkg->header));

    pkg->cmdID = VISION_CONTROL_CMD_ID;

    pkg->vision.yawDelta = yawDelta;
    pkg->vision.pitchDelta = pitchDelta;
    rm::appendCRC16CheckSum((uint8_t *) pkg.get(),
                            sizeof(pkg->header) + sizeof(pkg->cmdID) +
                            sizeof(pkg->vision) + sizeof(pkg->tail));

    ::tcflush(serial.lowest_layer().native_handle(), TCIFLUSH);  // clear input buffer
    boost::asio::async_write(serial, boost::asio::buffer(
            pkg.get(),
            sizeof(pkg->header) + sizeof(pkg->cmdID) + sizeof(pkg->vision) + sizeof(pkg->tail)),
                             [this, pkg](auto &error, auto numBytes) { handleSend(pkg, error, numBytes); }
    );

    return true;
}

void Serial::handleSend(std::shared_ptr<Package> pkg, const boost::system::error_code &error, size_t numBytes) {
    if (error) {
        std::cerr << "Serial: send error: " << error.message() << "\n";
        std::exit(1);
    }
    ++cumulativeFrameCounter;
}

}