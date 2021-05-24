//
// Created by liuzikai on 5/24/21.
//

#include "Serial.h"
#include <thread>
#include <iostream>

boost::asio::io_context ioContext;

std::thread ioThread([]{
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard(ioContext.get_executor());
    ioContext.run();  // this operation is blocking, until ioContext is deleted
});

float yawDelta = 0, pitchDelta = 0;

meta::Serial serial(ioContext);

std::thread sendThread([]{
    while (true) {
        serial.sendTargetAngles(yawDelta, pitchDelta);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
});

int main() {

    while (true) {
        std::cout << "Set new yawDelta and pitchDelta: ";
        std::cin >> yawDelta >> pitchDelta;
    }

    return 0;
}