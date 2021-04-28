//
// Created by liuzikai on 4/18/21.
//

#include "Executor.h"
#include "Camera.h"
#include "ArmorDetector.h"
#include "ImageDataManager.h"

namespace meta {

Executor::Executor(Camera *camera, ArmorDetector *detector, ImageDataManager *dataManager)
        : camera(camera), detector(detector), dataManager(dataManager),
        threadShouldExit(false), frameCounter(0) {

}

void Executor::applyParams(const ParamSet &p) {
    params = p;
    if (camera) {
        if (camera->isOpened()) camera->release();
        camera->open(params);
    }
    if (detector) {
        detector->setParams(params);
    }
}

int Executor::fetchAndClearFrameCounter() {
    int ret = frameCounter;
    frameCounter = 0;
    return ret;
}

void Executor::stop() {
    if (th) {  // executor is running
        threadShouldExit = true;
        th->join();
        delete th;
        th = nullptr;
    }
    curAction = NONE;
}

bool Executor::startRealTimeDetection() {
    if (th) stop();
    if (!camera) {
        std::cerr << "Executor: camera not set for REAL_TIME_DETECTION" << std::endl;
        return false;
    }
    if (!detector) {
        std::cerr << "Executor: detector not set for REAL_TIME_DETECTION" << std::endl;
        return false;
    }
    // Start real-time detection thread
    curAction = REAL_TIME_DETECTION;
    threadShouldExit = false;
    th = new std::thread(&Executor::runRealTimeDetection, this);
    return true;
}

bool Executor::startSingleImageDetection(const string &imageName) {
    if (th) stop();
    if (!detector) {
        std::cerr << "Executor: detector not set for REAL_TIME_DETECTION" << std::endl;
        return false;
    }
    if (!dataManager) {
        std::cerr << "Executor: dataManager not set for REAL_TIME_DETECTION" << std::endl;
        return false;
    }
    // Start real-time detection thread
    curAction = SINGLE_IMAGE_DETECTION;

    cv::Mat img = dataManager->getImage(imageName);
    if (img.rows != params.image_height() || img.cols != params.image_width()) {
        cv::resize(img, img, cv::Size(params.image_width(), params.image_height()));
    }

    detector->clearImages();  // clear data of last execution
    auto targets = detector->detect(img);

    // Discard the result for now
    (void) targets;

    curAction = NONE;
    return true;
}

void Executor::runRealTimeDetection() {
    frameCounter = 0;
    unsigned int lastFrameID = -1;
    while(!threadShouldExit) {

        // Wait for new frame
        while (lastFrameID == camera->getFrameID());
        lastFrameID = camera->getFrameID();

        // Run armor detection algorithm
        auto targets = detector->detect(camera->getFrame());

        // Discard the result for now
        (void) targets;

        frameCounter++;
    }
}

int Executor::loadDataSet(const string &path) {
    return dataManager->loadDataSet(path);
}

}