//
// Created by liuzikai on 3/11/21.
//

#include "Camera.h"
#include "ArmorDetector.h"
#include "DetectorTuner.h"
#include "TerminalSocket.h"
#include <iostream>

using namespace meta;

Camera camera;
ArmorDetector armorDetector;
DetectorTuner detectorTuner(&armorDetector, &camera);

SharedParameters sharedParams;
ArmorDetector::ParameterSet detectorParams;
Camera::ParameterSet cameraParams;

TerminalSocketServer socketServer(8800);

void handleCameraFrame(void *) {
    if (socketServer.connected()) {

        std::vector<uchar> buf;
        std::vector<int> param(2);
        param[0] = cv::IMWRITE_JPEG_QUALITY;
        param[1] = 80;
        cv::Mat outImage;
        cv::resize(camera.getFrame(), outImage, cv::Size(), 0.5, 0.5);
        cv::imencode(".jpg", outImage, buf, param);

        socketServer.sendBytes("camera", buf.data(), buf.size());
    }
}

void handleServerDisconnection(TerminalSocketServer* s) {
    std::cout << "TerminalSocketServer: disconnected" << std::endl;
    s->startAccept(handleServerDisconnection);
}


int main(int argc, char *argv[]) {

    socketServer.startAccept(handleServerDisconnection);

    camera.registerNewFrameCallBack(handleCameraFrame, nullptr);
    camera.open(sharedParams, cameraParams);

    while(true) {}

    return 0;
}