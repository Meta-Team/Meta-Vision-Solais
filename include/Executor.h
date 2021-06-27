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

    explicit Executor(Camera *camera, ImageSet *imageSet, VideoSet *videoSet, ParamSetManager *paramSetManager,
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

    void stopRecordToVideo();

    /** Execution **/

    enum Action {
        NONE,
        STREAMING_DETECTION,
        SINGLE_IMAGE_DETECTION
    };

    Action getCurrentAction() const { return curAction; }

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

    std::mutex &detectorOutputMutex() { return detector_->outputMutex; }

    std::mutex armorsOutputMutex;
    const std::vector<AimingSolver::ArmorInfo> &armorsOutput() const { return armorsOutput_; }

private:

    Camera *camera_;
    ImageSet *imageSet_;
    VideoSet *videoSet_;
    ArmorDetector *detector_;
    ParamSetManager *paramSetManager_;
    PositionCalculator *positionCalculator_;
    AimingSolver *aimingSolver_;
    Serial *serial_;

    InputSource *currentInput_ = nullptr;

    ParamSet params;

    Action curAction = NONE;

    std::thread *th = nullptr;
    bool threadShouldExit = false;

    std::vector<AimingSolver::ArmorInfo> armorsOutput_;

    void applyParams(const ParamSet &p);

    void runStreamingDetection(InputSource *source);

    cv::VideoWriter videoWriter;
    std::mutex videoWriterMutex;

};

}


#endif //META_VISION_SOLAIS_EXECUTOR_H
