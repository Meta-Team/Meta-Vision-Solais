//
// Created by liuzikai on 4/18/21.
//

#include "Executor.h"
#include <iostream>

namespace meta {

Executor::Executor(Camera *camera, ImageSet *imageSet, ArmorDetector *detector, ParamSetManager *paramSetManager,
                   PositionCalculator *positionCalculator, AimingSolver *aimingSolver, Serial *serial)
        : camera_(camera), imageSet_(imageSet), detector_(detector), paramSetManager_(paramSetManager),
          positionCalculator_(positionCalculator), aimingSolver_(aimingSolver), serial_(serial) {

    reloadLists();
}

void Executor::switchParamSet(const std::string &paramSetName) {
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

    // Local copy
    params = p;

    // Input
    // Skip re-opening the video source if the parameter doesn't change to save some time
    if (!(p.camera_id() == params.camera_id() &&
          p.fps() == params.fps() &&
          p.image_width() == params.image_width() &&
          p.image_height() == params.image_height() &&
          p.gamma().enabled() == params.gamma().enabled() &&
          p.gamma().val() == params.gamma().val() &&
          p.manual_exposure().enabled() == params.manual_exposure().enabled() &&
          p.manual_exposure().val() == params.manual_exposure().val())) {

        if (camera_->isOpened()) {
            camera_->close();
            camera_->open(params);
        }

        if (imageSet_->isOpened()) {
            imageSet_->close();
            imageSet_->open(params);
        }
    }

    // Detector
    detector_->setParams(params);

    // PositionCalculator
    {
        std::string filename =
                std::string(PARAM_SET_ROOT) + "/params/" +
                std::to_string(params.image_width()) + "x" + std::to_string(params.image_height()) + ".xml";

        cv::Mat cameraMatrix;
        cv::Mat distCoeffs;
        float zScale;

        cv::FileStorage fs(filename, cv::FileStorage::READ);
        if (!fs.isOpened()) {
            std::cerr << "Failed to open " << filename << std::endl;
            std::exit(1);
        }

        fs["cameraMatrix"] >> cameraMatrix;
        fs["distCoeffs"] >> distCoeffs;
        fs["zScale"] >> zScale;

        // TODO: large armor?
        positionCalculator_->setParameters(120, 55, cameraMatrix, distCoeffs, zScale);
    }

    // AimingSolver
    aimingSolver_->setParams(params);
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

bool Executor::startSingleImageDetection(const std::string &imageName) {
    if (th) stop();

    curAction = SINGLE_IMAGE_DETECTION;

    // Run on single image without starting the streaming thread
    cv::Mat img = imageSet_->getSingleImage(imageName, params);

    detector_->clearImages();  // clear data of last execution
    std::vector<ArmorDetector::DetectedArmor> detectedArmors = detector_->detect(img);

    // Solve armor positions
    std::vector<AimingSolver::ArmorInfo> armors;
    for (const auto &detectedArmor : detectedArmors) {
        cv::Point3f offset;
        if (positionCalculator_->solve(detectedArmor.points, offset)) {
            armors.emplace_back(AimingSolver::ArmorInfo{
                    detectedArmor.points,
                    detectedArmor.center,
                    offset
            });
        }
    }

    // Update
    aimingSolver_->update(armors);

    // Output armors
    if (armorsOutputMutex.try_lock()) {
        armorsOutput_ = armors;
        armorsOutputMutex.unlock();
    }
    // Otherwise, simply discard the results

    /*if (aimingSolver_->shouldSendControlCommand()) {
        // Send control command
        const auto &command = aimingSolver_->getControlCommand();
        serial_->sendTargetAngles(command.yawDelta, command.pitchDelta);
    }*/

    // Increment statistics
    ++cumulativeFrameCounter;

    // Do not set curAction to NONE so that the result can be fetched
    // curAction = NONE;
    return true;
}

void Executor::runStreamingDetection(VideoSource *source) {
    std::cout << "Executor: start streaming\n";

    currentInput_ = source;
    currentInput_->fetchAndClearFrameCounter();
    // TODO: clear the solver

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
        std::vector<ArmorDetector::DetectedArmor> detectedArmors = detector_->detect(img);

        // Solve armor positions
        std::vector<AimingSolver::ArmorInfo> armors;
        for (const auto &detectedArmor : detectedArmors) {
            cv::Point3f offset;
            if (positionCalculator_->solve(detectedArmor.points, offset)) {
                armors.emplace_back(AimingSolver::ArmorInfo{
                        detectedArmor.points,
                        detectedArmor.center,
                        offset
                });
            }
        }

        // Update
        aimingSolver_->update(armors);

        // Output armors
        if (armorsOutputMutex.try_lock()) {
            armorsOutput_ = armors;
            armorsOutputMutex.unlock();
        }
        // Otherwise, simply discard the results

        if (serial_ && aimingSolver_->shouldSendControlCommand()) {
            // Send control command
            const auto &command = aimingSolver_->getControlCommand();
            serial_->sendTargetAngles(command.yawDelta, command.pitchDelta);
        }

        // Increment frame counter
        cumulativeFrameCounter++;
    }

    std::cout << "Executor: stopped\n";

    source->close();
    currentInput_ = nullptr;
    curAction = NONE;
}

void Executor::reloadLists() {
    imageSet_->reloadImageSetList();
    paramSetManager_->reloadParamSetList();  // switch to default parameter set
    applyParams(paramSetManager_->loadCurrentParamSet());
}

int Executor::switchImageSet(const std::string &path) {
    return imageSet_->switchImageSet(path);
}

unsigned int Executor::fetchAndClearInputFrameCounter() {
    VideoSource *input = currentInput_;  // make a copy
    if (input) {
        return input->fetchAndClearFrameCounter();
    } else {
        return 0;
    }
}

std::string Executor::captureImageFromCamera() {
    if (!camera_->isOpened()) {
        if (!camera_->open(params)) {
            return "[Error: failed to open camera]";
        }
    }
    cv::Mat img = camera_->getFrame();  // no actual data copy
    return imageSet_->saveCapturedImage(img, params);
}

}