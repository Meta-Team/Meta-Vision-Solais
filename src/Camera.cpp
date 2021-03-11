//
// Created by liuzikai on 3/11/21.
//

#include "Camera.h"
#include <iostream>

namespace meta {

bool Camera::open(const SharedParameters &shared, const Camera::ParameterSet &params) {
    sharedParams = shared;
    capParams = params;

    capInfoSS.clear();

    // Open the camera
    cap.open(params.cameraID, cv::CAP_ANY);
    if (!cap.isOpened()) {
        capInfoSS << "Failed to open camera " << params.cameraID << "\n";
        std::cerr << capInfoSS.rdbuf();
        return false;
    }

    // Set parameters
    if (!cap.set(cv::CAP_PROP_FRAME_WIDTH, shared.imageWidth)) {
        capInfoSS << "Failed to set width.\n";
    }
    if (!cap.set(cv::CAP_PROP_FRAME_HEIGHT, shared.imageHeight)) {
        capInfoSS << "Failed to set height.\n";
    }
    if (!cap.set(cv::CAP_PROP_FPS, params.fps)) {
        capInfoSS << "Failed to set fps.\n";
    }
    if (params.enableGamma) {
        if (!cap.set(cv::CAP_PROP_GAMMA, params.gamma)) {
            capInfoSS << "Failed to set gamma.\n";
        }
    }

    // Get a test frame
    cap.read(buffer[0]);
    if (buffer->empty()) {
        capInfoSS << "Failed to fetch test image from camera " << params.cameraID << "\n";
        std::cerr << capInfoSS.rdbuf();
        return false;
    }
    if (buffer[0].cols != sharedParams.imageWidth || buffer[0].rows != shared.imageHeight) {
        capInfoSS << "Invalid frame size. "
                  << "Expected: " << sharedParams.imageWidth << "x" << sharedParams.imageHeight << ", "
                  << "Actual: " << buffer[0].cols << "x" << buffer[0].rows << "\n";
        std::cerr << capInfoSS.rdbuf();
        return false;
    }

    // Report actual parameters
    capInfoSS << "Camera " << params.cameraID << ", "
              << cap.get(cv::CAP_PROP_FRAME_WIDTH) << "x" << cap.get(cv::CAP_PROP_FRAME_HEIGHT)
              << " @ " << cap.get(cv::CAP_PROP_FPS) << " fps\n"
              << "Gamma: " << cap.get(cv::CAP_PROP_GAMMA) << "\n";
    std::cout << capInfoSS.rdbuf();

    // Start the fetching thread
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