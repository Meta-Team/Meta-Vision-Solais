//
// Created by liuzikai on 4/18/21.
//

#include "Executor.h"
#include "Camera.h"
#include "ImageSet.h"
#include "ArmorDetector.h"
#include "ParamSetManager.h"

namespace meta {

Executor::Executor(Camera *camera, ImageSet *imageSet, ArmorDetector *detector, ParamSetManager *paramSetManager)
        : camera_(camera), imageSet_(imageSet), detector_(detector), paramSetManager_(paramSetManager),
          threadShouldExit(false), frameCounter(0) {

    reloadLists();
}

void Executor::switchParamSet(const string &paramSetName) {
    paramSetManager_->switchToParamSet(paramSetName);
    applyParams(paramSetManager_->loadCurrentParamSet());
}

void Executor::saveAndApplyParams(const ParamSet &p) {
    paramSetManager_->saveCurrentParamSet(p);
    applyParams(p);
}

void Executor::applyParams(const ParamSet &p) {
    // For better user experience for parameter tuning, here we don't stop the detection thread
    // But if there is some changes in the streaming source, detection may terminate anyway

    params = p;

    // Skip re-opening the video source if the parameter doesn't change to save some time
    if (!(p.camera_id() == params.camera_id() && p.fps() == p.fps() &&
          p.image_width() == params.image_width() && p.image_height() == params.image_height() &&
          p.gamma().enabled() == p.gamma().enabled() && p.gamma().val() == p.gamma().val())) {

        if (camera_->isOpened()) {
            camera_->close();
            camera_->open(params);
        }

        if (imageSet_->isOpened()) {
            imageSet_->close();
            imageSet_->open(params);
        }
    }
    detector_->setParams(p);
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

    if (!camera_->isOpened()) {
        if (!camera_->open(params)) {
            return false;
        }
    }

    // Start real-time detection thread
    curAction = REAL_TIME_DETECTION;
    threadShouldExit = false;
    th = new std::thread(&Executor::runStreamingDetection, this, camera_);
    return true;
}

bool Executor::startImageSetDetection() {
    if (th) stop();

    if (!imageSet_->isOpened()) imageSet_->close();  // always restart
    if (!imageSet_->open(params)) {
        return false;
    }

    // Start real-time detection thread
    curAction = REAL_TIME_DETECTION;
    threadShouldExit = false;
    th = new std::thread(&Executor::runStreamingDetection, this, imageSet_);
    return true;
}

bool Executor::startSingleImageDetection(const string &imageName) {
    if (th) stop();

    curAction = SINGLE_IMAGE_DETECTION;

    // Run on single image without starting the streaming thread
    cv::Mat img = imageSet_->getSingleImage(imageName, params);

    detector_->clearImages();  // clear data of last execution
    auto targets = detector_->detect(img);

    // Discard the result for now
    (void) targets;

    curAction = NONE;
    return true;
}

void Executor::runStreamingDetection(VideoSource *source) {
    frameCounter = 0;

    unsigned int lastFrameID = source->getFrameID();  // use last frame ID to wait for new frame
    while (true) {

        // Wait for new frame
        while (!threadShouldExit && lastFrameID == source->getFrameID());

        if (threadShouldExit || source->getFrameID() == -1) {
            break;
        }
        lastFrameID = source->getFrameID();  // update frame ID

        auto &img = source->getFrame();  // no need for copying due to double buffering
        source->fetchNextFrame();

        // Run armor detection algorithm
        auto targets = detector_->detect(img);

        // Discard the result for now
        (void) targets;

        // Increment frame counter
        frameCounter++;
    }

    std::cout << "Executor: stopped\n";

    source->close();
    curAction = NONE;
}

void Executor::reloadLists() {
    imageSet_->reloadImageSetList();
    paramSetManager_->reloadParamSetList();  // switch to default parameter set
    applyParams(paramSetManager_->loadCurrentParamSet());
}

int Executor::switchImageSet(const string &path) {
    return imageSet_->switchImageSet(path);
}

}