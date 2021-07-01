//
// Created by liuzikai on 4/18/21.
//

#include "Executor.h"
#include "Utilities.h"
#include <iostream>

namespace meta {

Executor::Executor(Camera *camera, ImageSet *imageSet, VideoSet *videoSet, ParamSetManager *paramSetManager,
                   ArmorDetector *detector, PositionCalculator *positionCalculator, AimingSolver *aimingSolver,
                   Serial *serial)
        : camera_(camera), imageSet_(imageSet), videoSet_(videoSet), paramSetManager_(paramSetManager),
          detector_(detector), positionCalculator_(positionCalculator), aimingSolver_(aimingSolver), serial_(serial) {

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
    }
    if (imageSet_->isOpened()) {
        stop();
        imageSet_->close();
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

        positionCalculator_->setParameters(
                {(float) params.small_armor_size().x(), (float) params.small_armor_size().y()},
                {(float) params.large_armor_size().x(), (float) params.large_armor_size().y()},
                cameraMatrix, distCoeffs, zScale);
    }

    // AimingSolver
    aimingSolver_->setParams(params);  // reset history inside
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
    curAction = STREAMING_DETECTION;
    threadShouldExit = false;
    th = new std::thread(&Executor::runStreamingDetection, this, camera_);
    return true;
}

bool Executor::startImageSetDetection() {
    if (th) stop();

    if (imageSet_->isOpened()) imageSet_->close();  // always restart
    if (!imageSet_->openCurrentImageSet(params)) return false;

    // Start real-time detection thread
    curAction = STREAMING_DETECTION;
    threadShouldExit = false;
    th = new std::thread(&Executor::runStreamingDetection, this, imageSet_);
    return true;
}

bool Executor::startSingleImageDetection(const std::string &imageName) {
    if (th) stop();

    if (imageSet_->isOpened()) imageSet_->close();
    if (!imageSet_->openSingleImage(imageName, params)) return false;

    curAction = SINGLE_IMAGE_DETECTION;
    threadShouldExit = false;
    th = new std::thread(&Executor::runStreamingDetection, this, imageSet_);
    return true;
}

bool Executor::startVideoDetection(const std::string &videoName) {
    if (th) stop();

    if (videoSet_->isOpened()) videoSet_->close();
    if (!videoSet_->openVideo(videoName, params)) return false;

    curAction = STREAMING_DETECTION;
    threadShouldExit = false;
    th = new std::thread(&Executor::runStreamingDetection, this, videoSet_);
    return true;
}

void Executor::runStreamingDetection(InputSource *source) {
    std::cout << "Executor: start streaming\n";

    currentInput_ = source;
    currentInput_->fetchAndClearFrameCounter();
    aimingSolver_->resetHistory();

    TimePoint lastFrameTime = TimePoint();  // use last frame capture time to wait for new frame
    TimePoint frameTime = TimePoint();
    while (true) {

        // Wait for new frame, fetch and store time first and then compare
        while (!threadShouldExit && lastFrameTime == (frameTime = source->getFrameCaptureTime()));

        if (threadShouldExit || frameTime == TimePoint()) {
            break;
        }
        lastFrameTime = frameTime;

        auto &img = source->getFrame();  // no need for deep copying
        source->fetchNextFrame();


        // Run armor detection algorithm
        std::vector<ArmorDetector::DetectedArmor> detectedArmors = detector_->detect(img);

        // Solve armor positions
        std::vector<AimingSolver::DetectedArmorInfo> armors;
        for (const auto &detectedArmor : detectedArmors) {
            cv::Point3f offset, rotation;
            if (positionCalculator_->solve(detectedArmor.points, detectedArmor.largeArmor, offset, rotation)) {
                armors.emplace_back(AimingSolver::DetectedArmorInfo{
                        detectedArmor.points,
                        detectedArmor.center,
                        offset,
                        rotation,
                        detectedArmor.largeArmor,
                        detectedArmor.number
                });
            }
        }

        // Update
        aimingSolver_->updateArmors(armors, frameTime);

        if (serial_ && aimingSolver_->shouldSendControlCommand()) {
            // Send control command
            const auto &command = aimingSolver_->getControlCommand();
            serial_->sendControlCommand(
                    {(command.mode == AimingSolver::RELATIVE_ANGLE ? Serial::RELATIVE_ANGLE : Serial::ABSOLUTE_ANGLE),
                     -command.yaw, command.pitch});  // notice the minus sign
        }

        // Assign (no copying) results all at once, if the result is not being processed
        if (outputMutex.try_lock()) {
            originalOutput = detector_->imgOriginal;
            brightnessOutput = detector_->imgBrightness;
            colorOutput = detector_->imgColor;
            contoursOutput = detector_->imgContours;

            armorsOutput = armors;

            aimingSolver_->gimbalAngleMutex.lock();
            {
                currentGimbalOutput.x = aimingSolver_->yawCurrentAngle;  // notice the minus sign
                currentGimbalOutput.y = aimingSolver_->pitchCurrentAngle;
            }
            aimingSolver_->gimbalAngleMutex.unlock();

            outputMutex.unlock();
        }
        // Otherwise, simple discard the results of current run

        // Increment frame counter
        cumulativeFrameCounter++;
    }

    std::cout << "Executor: stopped\n";

    source->close();
    currentInput_ = nullptr;
    if (curAction != SINGLE_IMAGE_DETECTION) {  // do not reset SINGLE_IMAGE_DETECTION for result fetching
        curAction = NONE;
    }
}

void Executor::reloadLists() {
    imageSet_->reloadImageSetList();
    videoSet_->reloadVideoList();
    paramSetManager_->reloadParamSetList();  // switch to default parameter set
    applyParams(paramSetManager_->loadCurrentParamSet());
}

int Executor::switchImageSet(const std::string &path) {
    return imageSet_->switchImageSet(path);
}

unsigned int Executor::fetchAndClearInputFrameCounter() {
    InputSource *input = currentInput_;  // make a copy
    if (input) {
        return input->fetchAndClearFrameCounter();
    } else {
        if (camera_->isRecordingVideo()) {
            return camera_->fetchAndClearFrameCounter();
        } else {
            return 0;
        }
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

std::string Executor::startRecordToVideo() {
    if (!camera_->isOpened()) {
        if (!camera_->open(params)) {
            return "[Error: failed to open camera]";
        }
    }
    if (camera_->getFPS() == 0) {
        return "[Error: camera reported 0 FPS]";
    }

    // Filename: <width>_<height>_<fps>_<target color>_<time stamp>.avi
    fs::path filename = videoSet_->videoSetRoot / fs::path(
            std::to_string(params.image_width()) + "_" + std::to_string(params.image_height()) + "_" +
            std::to_string(camera_->getFPS()) + "_" +
            (params.enemy_color() == package::ParamSet_EnemyColor_BLUE ? "blue" : "red") + "_" + currentTimeString() +
            ".avi");

    if (!camera_->startRecordToVideo(filename.string(), cv::Size(params.image_width(), params.image_height()))) {
        return "[Error: failed to open videoWriter]";
    }

    return filename.string();
}

bool Executor::hasOutputs() {
    if (curAction == SINGLE_IMAGE_DETECTION) {
        curAction = NONE;  // reset
        return true;
    }
    if (curAction != NONE) return true;
    if (camera_->isRecordingVideo()) return true;
    return false;
}

void Executor::fetchOutputs(cv::Mat &originalImage, cv::Mat &brightnessImage, cv::Mat &colorImage,
                            cv::Mat &contourImage, std::vector<AimingSolver::DetectedArmorInfo> &armors,
                            cv::Point2f &currentGimbal) {
    if (curAction != NONE) {
        outputMutex.lock();
        {
            originalImage = originalOutput;
            brightnessImage = brightnessOutput;
            colorImage = colorOutput;
            contourImage = contoursOutput;
            armors = armorsOutput;
            currentGimbal = currentGimbalOutput;
        }
        outputMutex.unlock();

    } else {
        if (camera_->isRecordingVideo()) {
            originalImage = camera_->getFrame();
        }
    }
}

}