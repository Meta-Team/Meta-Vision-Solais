//
// Created by liuzikai on 4/18/21.
//

#include "Executor.h"
#include "Camera.h"
#include "ArmorDetector.h"

namespace meta {

Executor::Executor(Camera *camera, ArmorDetector *detector)
        : camera(camera), detector(detector), threadShouldExit(false), frameCounter(0) {

}

void Executor::setAction(Executor::Action action) {
    if (action == curAction) return;
    if (th) {  // executor is running
        threadShouldExit = true;
        th->join();
        delete th;
        th = nullptr;
    }
    curAction = action;
    threadShouldExit = false;
    switch (curAction) {
        case NONE:
            // Do nothing
            break;
        case REAL_TIME_DETECTION:
            // Start real-time detection thread
            th = new std::thread(&Executor::runRealTimeDetection, this);
            break;
    }
}

int Executor::fetchAndClearFrameCounter() {
    int ret = frameCounter;
    frameCounter = 0;
    return ret;
}

void Executor::runRealTimeDetection() {
    frameCounter = 0;
    unsigned int lastFrameID = -1;
    while(!threadShouldExit) {

        // Wait for new frame
        while (lastFrameID == camera->getFrameID());

        // Run armor detection algorithm
        auto targets = detector->detect(camera->getFrame());

        // Discard the result for now
        (void) targets;

        frameCounter++;
    }
}

}