//
// Created by liuzikai on 3/11/21.
//

#include "Camera.h"
#include <iostream>

namespace meta {

bool Camera::open(const SharedParameters &shared, const Camera::ParameterSet &params) {
    sharedParams = shared;
    capParams = params;

    cap.open(params.cameraID, cv::CAP_ANY);
    if (!cap.isOpened()) {
        std::cerr << "Failed to open camera " << params.cameraID << "\n";
        return false;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, shared.imageWidth);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, shared.imageHeight);

    // Get a test frame
    cap.read(buffer[0]);
    if (buffer->empty()) {
        std::cerr << "Failed to fetch test image from camera " << params.cameraID << "\n";
        return false;
    }
    if (buffer[0].cols != sharedParams.imageWidth || buffer[0].rows != shared.imageHeight) {
        std::cerr << "Invalid frame size. "
                  << "Expected: " << sharedParams.imageWidth << "x" << sharedParams.imageHeight << ", "
                  << "Actual: " << buffer[0].cols << "x" << buffer[0].rows << "\n";
        return false;
    }

    std::cout << "Camera " << params.cameraID << " "
              << cap.get(cv::CAP_PROP_FRAME_WIDTH) << "x" << cap.get(cv::CAP_PROP_FRAME_HEIGHT)
              << " @ " << cap.get(cv::CAP_PROP_FPS) << " fps\n";

    if (th) {
        release();
    }
    threadShouldExit = false;
    th = new std::thread(&Camera::readFrameFromCamera, this);

    return true;
}

Camera::~Camera() {
    release();
}

void Camera::registerNewFrameCallBack(Camera::NewFrameCallBack callBack, void *param) {
    callbacks.emplace_back(callBack, param);
}

void Camera::readFrameFromCamera() {

    while(!threadShouldExit) {

        int workingBuffer = 1 - lastBuffer;

        if (!cap.isOpened()) {
            break;
        }

        if (!cap.read(buffer[workingBuffer])) {
            continue;
        }

        bufferFrameID[workingBuffer] = bufferFrameID[lastBuffer] + 1;
        lastBuffer = workingBuffer;

        for (const auto &item : callbacks) {
            item.first(item.second);
        }

    }

    cap.release();
    std::cout << "Camera " << capParams.cameraID << " closed\n";
}

void Camera::release() {
    if (th) {
        threadShouldExit = true;
        th->join();
        delete th;
        th = nullptr;
    }
}

}