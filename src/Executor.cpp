//
// Created by liuzikai on 4/18/21.
//

#include "Executor.h"
#include "Camera.h"
#include "ArmorDetector.h"
#include "ImageDataManager.h"

namespace meta {

Executor::Executor(Camera *camera, ArmorDetector *detector, ImageDataManager *dataManager)
        : camera_(camera), detector_(detector), dataManager_(dataManager),
          threadShouldExit(false), frameCounter(0) {

}

void Executor::applyParams(const ParamSet &p) {
    // Skip re-opening the camera_ if the parameter doesn't change to save some time
    if (camera_ && !(p.camera_id() == params.camera_id() && p.fps() == p.fps() &&
                    p.image_width() == params.image_width() && p.image_height() != params.image_height() &&
                    p.gamma().enabled() == p.gamma().enabled() && p.gamma().val() == p.gamma().val())) {
        if (camera_->isOpened()) camera_->release();
        camera_->open(p);
    }
    if (detector_) {
        detector_->setParams(p);
    }
    params = p;
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
    if (!camera_) {
        std::cerr << "Executor: camera_ not set for REAL_TIME_DETECTION" << std::endl;
        return false;
    }
    if (!detector_) {
        std::cerr << "Executor: detector_ not set for REAL_TIME_DETECTION" << std::endl;
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
    if (!detector_) {
        std::cerr << "Executor: detector_ not set for REAL_TIME_DETECTION" << std::endl;
        return false;
    }
    if (!dataManager_) {
        std::cerr << "Executor: dataManager_ not set for REAL_TIME_DETECTION" << std::endl;
        return false;
    }
    // Start real-time detection thread
    curAction = SINGLE_IMAGE_DETECTION;

    cv::Mat img = dataManager_->getImage(imageName);
    if (img.rows != params.image_height() || img.cols != params.image_width()) {
        cv::resize(img, img, cv::Size(params.image_width(), params.image_height()));
    }

    detector_->clearImages();  // clear data of last execution
    auto targets = detector_->detect(img);

    // Discard the result for now
    (void) targets;

    curAction = NONE;
    return true;
}

void Executor::runRealTimeDetection() {
    frameCounter = 0;
    unsigned int lastFrameID = -1;
    while (!threadShouldExit) {

        // Wait for new frame
        while (lastFrameID == camera_->getFrameID());
        lastFrameID = camera_->getFrameID();

        // Run armor detection algorithm
        auto targets = detector_->detect(camera_->getFrame());

        // Discard the result for now
        (void) targets;

        frameCounter++;
    }
}

void Executor::reloadLists() {
    dataManager_->reloadDataSetList();
}

int Executor::loadImageDataSet(const string &path) {
    return dataManager_->loadDataSet(path);
}

}