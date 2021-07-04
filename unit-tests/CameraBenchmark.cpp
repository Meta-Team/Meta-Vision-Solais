//
// Created by liuzikai on 5/28/21.
//

#include "Camera.h"
#include "Parameters.h"
#include <iostream>
#include <thread>
#include <opencv2/core/utility.hpp>
#include <opencv2/core/cuda.hpp>

using namespace meta;
using namespace package;

std::unique_ptr<MVCamera> camera;
ParamSet params;

std::thread consumer;

int main(int argc, char *argv[]) {

    std::cout << "CUDA device count: " << cv::cuda::getCudaEnabledDeviceCount() << std::endl;
    std::cout << cv::getBuildInformation() << std::endl;

    CameraSdkInit(0);

    camera = std::make_unique<MVCamera>();

    params.set_camera_id(0);
    params.set_image_width(1280);
    params.set_image_height(1024);
    params.set_fps(211);
    params.set_allocated_gamma(allocToggledFloat(true, 50));
    params.set_allocated_manual_exposure(allocToggledInt(true, 1000));

    if (!camera->open(params)) {
        std::cerr << "Failed to open camera" << std::endl;
        return 1;
    }
    std::cout << camera->getCameraInfo() << std::endl;

    consumer = std::thread([] {
        TimePoint lastFrameTime = 0;
        while (true) {
            while (camera->getFrameCaptureTime() == lastFrameTime) std::this_thread::yield();
            lastFrameTime = camera->getFrameCaptureTime();
            camera->fetchNextFrame();
        }
    });

    int count = 0;
    while (true) {
        std::cout << "FPS: " << camera->fetchAndClearFrameCounter() * 5 << " frames/s" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds (200));
        count++;
        if (count == 50) {
            tSdkImageResolution resolution;
            CameraGetImageResolution(camera->hCamera, &resolution);

            resolution.iIndex = 0xFF;  // customized ROI
            int width = params.image_width() / 2;
            int height = params.image_height() / 2;
            resolution.iHOffsetFOV = (1280 - width) / 2;
            resolution.iVOffsetFOV = (1024 - height) / 2;
            resolution.iWidthFOV = resolution.iWidth = width;
            resolution.iHeightFOV = resolution.iHeight = height;

            auto res = CameraSetImageResolution(camera->hCamera, &resolution);
            if (res != CAMERA_STATUS_SUCCESS) {
                std::cerr << "MVCamera: CameraSetImageResolution returned " << res << std::endl;
            } else {
                std::cout << "ROI enabled " << width << "x" << height;
            }
        }
    }

    return 0;
}