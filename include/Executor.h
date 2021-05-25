//
// Created by liuzikai on 4/18/21.
//

#ifndef META_VISION_SOLAIS_EXECUTOR_H
#define META_VISION_SOLAIS_EXECUTOR_H

// Include as few modules as possible and use forward declarations
#include "Parameters.h"
#include "VideoSource.h"
#include <thread>

namespace meta {

// Forward declarations
class Camera;
class ImageSet;
class ArmorDetector;
class ParamSetManager;

class Executor {
public:

    explicit Executor(Camera *camera, ImageSet *imageSet, ArmorDetector *detector, ParamSetManager *paramSetManager);

    const Camera *camera() const { return camera_; }
    const ImageSet *imageSet() const { return imageSet_; }
    const ArmorDetector *detector() const { return detector_; }
    const ParamSetManager *dataManager() const { return paramSetManager_; };

    void switchParamSet(const std::string &paramSetName);

    void saveAndApplyParams(const ParamSet &p);

    const ParamSet &getCurrentParams() const { return params; }

    enum Action {
        NONE,
        REAL_TIME_DETECTION,
        SINGLE_IMAGE_DETECTION
    };

    Action getCurrentAction() const { return curAction; }

    void stop();

    bool startRealTimeDetection();

    bool startSingleImageDetection(const std::string &imageName);

    bool startImageSetDetection();

    int fetchAndClearFrameCounter();

    void reloadLists();

    int switchImageSet(const std::string &path);

private:

    Camera *camera_;
    ImageSet *imageSet_;
    ArmorDetector *detector_;
    ParamSetManager *paramSetManager_;

    ParamSet params;

    Action curAction = NONE;

    std::thread *th = nullptr;
    std::atomic<bool> threadShouldExit;
    std::atomic<int> frameCounter;

    void applyParams(const ParamSet &p);

    void runStreamingDetection(VideoSource *source);

};

}


#endif //META_VISION_SOLAIS_EXECUTOR_H
