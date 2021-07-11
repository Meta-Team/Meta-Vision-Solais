//
// Created by liuzikai on 4/18/21.
//

#ifndef META_VISION_SOLAIS_EXECUTOR_H
#define META_VISION_SOLAIS_EXECUTOR_H

// Include as few modules as possible and use forward declarations
#include "Parameters.h"
#include "InputSource.h"
#include "Camera.h"
#include "ImageSet.h"
#include "VideoSet.h"
#include "ArmorDetector.h"
#include "ParamSetManager.h"
#include "PositionCalculator.h"
#include "AimingSolver.h"
#include "Serial.h"
#include <thread>

namespace meta {

class Executor : protected FrameCounterBase /* we would like to rename the function */ {
public:

    explicit Executor(OpenCVCamera *openCvCamera, MVCamera *mvCamera, ImageSet *imageSet, VideoSet *videoSet,
                      ParamSetManager *paramSetManager,
                      ArmorDetector *detector, PositionCalculator *positionCalculator, AimingSolver *aimingSolver,
                      Serial *serial);

    /** Read-Only Components **/

    const Camera *camera() const { return camera_; }

    const ImageSet *imageSet() const { return imageSet_; }

    const VideoSet *videoSet() const { return videoSet_; }

    const InputSource *currentInputSource() const { return currentInput_; };

    const ArmorDetector *detector() const { return detector_; }

    const ParamSetManager *dataManager() const { return paramSetManager_; };

    const PositionCalculator *positionCalculator() const { return positionCalculator_; }

    const AimingSolver *aimingSolver() const { return aimingSolver_; }

    const Serial *serial() const { return serial_; }

    /** Parameter Sets, Image List, and Video List Control **/

    void reloadLists();

    int switchImageSet(const std::string &path);

    void switchParamSet(const std::string &paramSetName);

    void saveAndApplyParams(const ParamSet &p);

    const ParamSet &getCurrentParams() const { return params; }

    cv::Mat getVideoPreview(const std::string &videoName) const { return videoSet_->getVideoFirstFrame(videoName, params); }

    /** Capture and Record **/

    std::string captureImageFromCamera();

    std::string startRecordToVideo();

    void stopRecordToVideo() { camera_->stopRecordToVideo(); }

    /** Execution **/

    /**
     * Stop execution.
     */
    void stop();

    /**
     * Start continuous detection on camera.
     * @return Success or not.
     */
    bool startRealTimeDetection();

    /**
     * Run detection on a single image. This operation is blocking and sets current action to SINGLE_IMAGE_DETECTION,
     * without resetting to NONE after completion. TCP socket can sent the result based on getCurrentAction() != NONE,
     * but should call stop() if the current action is SINGLE_IMAGE_DETECTION.
     * @param imageName
     * @return Success or not.
     */
    bool startSingleImageDetection(const std::string &imageName);

    /**
     * Start continuous detection on current image set.
     * @return  Success or not.
     */
    bool startImageSetDetection();

    /**
     * Start continuous detection on video.
     * @param videoName
     * @return  Success or not.
     */
    bool startVideoDetection(const std::string &videoName);

    /** Statistics and Output **/

    unsigned int fetchAndClearExecutorFrameCounter() { return FrameCounterBase::fetchAndClearFrameCounter(); }

    unsigned int fetchAndClearInputFrameCounter();

    unsigned int fetchAndClearSerialFrameCounter() { return serial_ ? serial_->fetchAndClearFrameCounter() : 0; }

    bool hasOutputs();

    /**
     * Fetch image outputs. Outputs are guaranteed to be completed and from the same detection pipeline. This function
     * can be called from another thread than the detection thread.
     * @param originalImage
     * @param brightnessImage
     * @param colorImage
     * @param lightsImage
     * @param lightRects
     * @param armors
     */
    void fetchOutputs(cv::Mat &originalImage, cv::Mat &brightnessImage, cv::Mat &colorImage, cv::Mat &lightsImage,
                      std::vector<cv::RotatedRect> &lightRects, std::vector<AimingSolver::ArmorInfo> &armors,
                      bool &tkTriggered, std::deque<AimingSolver::PulseInfo> &tkPulses);

private:

    OpenCVCamera *openCvCamera_;
    MVCamera *mvCamera_;
    Camera *camera_ = nullptr;
    ImageSet *imageSet_;
    VideoSet *videoSet_;
    ArmorDetector *detector_;
    ParamSetManager *paramSetManager_;
    PositionCalculator *positionCalculator_;
    AimingSolver *aimingSolver_;
    Serial *serial_;

    InputSource *currentInput_ = nullptr;

    ParamSet params;

    enum Action {
        NONE,
        STREAMING_DETECTION,
        SINGLE_IMAGE_DETECTION
    };

    Action curAction = NONE;

    std::thread *th = nullptr;
    bool threadShouldExit = false;

    void applyParams(const ParamSet &p);

    void runStreamingDetection(InputSource *source);

    std::mutex outputMutex;

    // Output Mats are assigned (no copying) after a completed detection so there is no intermediate result
    cv::Mat originalOutput;
    cv::Mat grayOutput;
    cv::Mat brightnessOutput;
    cv::Mat colorOutput;
    cv::Mat lightsImageOutput;

    std::vector<cv::RotatedRect> lightRectsOutput;
    std::vector<AimingSolver::ArmorInfo> armorsOutput;
    bool tkTriggeredOutput;
    std::deque<AimingSolver::PulseInfo> tkPulsesOutput;
};

}


#endif //META_VISION_SOLAIS_EXECUTOR_H
