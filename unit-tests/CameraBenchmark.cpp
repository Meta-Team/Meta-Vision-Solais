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

std::unique_ptr<Camera> camera;
ParamSet params;

int main(int argc, char *argv[]) {

    std::cout << "CUDA device count: " << cv::cuda::getCudaEnabledDeviceCount() << std::endl;
    std::cout << cv::getBuildInformation() << std::endl;

    camera = std::make_unique<Camera>();

    params.set_camera_id(0);
    params.set_image_width(1280);
    params.set_image_height(720);
    params.set_fps(120);
    params.set_allocated_gamma(allocToggledDouble(false));

    if (!camera->open(params)) {
        std::cerr << "Failed to open camera" << std::endl;
        return 1;
    }
    std::cout << camera->getCapInfo() << std::endl;

    while (true) {
        std::cout << "FPS: " << camera->fetchAndClearFrameCounter() << " frames/s" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }


    return 0;
}