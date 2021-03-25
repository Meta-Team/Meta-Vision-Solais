//
// Created by liuzikai on 3/11/21.
//

#include "Camera.h"
#include "ArmorDetector.h"
#include "DetectorTuner.h"
#include "TerminalSocket.h"
#include "TerminalParameters.h"
#include <iostream>

using namespace meta;

Camera camera;
ArmorDetector armorDetector;
DetectorTuner detectorTuner(&armorDetector, &camera);

SharedParameters sharedParams;
ArmorDetector::ParameterSet detectorParams;
Camera::ParameterSet cameraParams;

// Setup a server with automatic acceptance
TerminalSocketServer socketServer(8800, [](auto s) {
    std::cout << "TerminalSocketServer: disconnected" << std::endl;
    s->startAccept();
});

void sendCameraFrame() {
    cv::Mat outImage;
    std::vector<uchar> buf;
    float ratio = (float) TERMINAL_IMAGE_PREVIEW_HEIGHT / (float) camera.getFrame().rows;

    cv::resize(camera.getFrame(), outImage, cv::Size(), ratio, ratio);
    cv::imencode(".jpg", outImage, buf, {cv::IMWRITE_JPEG_QUALITY, 80});

    socketServer.sendBytes("cameraImage", buf.data(), buf.size());
}

void handleRecvSingleString(std::string_view name, std::string_view s) {
    if (name == "camera") {
        if (s == "toggle") {
            // Toggle camera
            if (camera.isOpened()) {

                camera.release();
                socketServer.sendSingleString("message", "Camera closed");

            } else {
                camera.open(sharedParams, cameraParams);

                socketServer.sendSingleString("message", "Camera opened");

                // Send camera information
                socketServer.sendSingleString("cameraInfo", camera.getCapInfo());

                // Send a frame to start the fetching cycle
                sendCameraFrame();
            }
        } else if (s == "fetch") {

            // Fetch a frame from the camera
            sendCameraFrame();

        } else goto INVALID_COMMAND;
    }

    return;
INVALID_COMMAND:
    std::cerr << "Invalid single-string package <" << name << "> \"" << s << "\"" << std::endl;
}

int main(int argc, char *argv[]) {

    socketServer.startAccept();
    socketServer.setCallbacks(handleRecvSingleString,
                              nullptr,
                              nullptr,
                              nullptr);

    while(true) {}

    return 0;
}