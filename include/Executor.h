//
// Created by liuzikai on 4/18/21.
//

#ifndef META_VISION_SOLAIS_EXECUTOR_H
#define META_VISION_SOLAIS_EXECUTOR_H

#include "Common.h"
#include "Parameters.h"
#include <thread>

namespace meta {

// Forward declarations
class Camera;
class ArmorDetector;
class ImageDataManager;

class Executor {
public:

    explicit Executor(Camera *camera, ArmorDetector *detector, ImageDataManager *dataManager);

    const Camera *camera() const { return camera_; }
    const ArmorDetector *detector() const { return detector_; }
    const ImageDataManager *dataManager() const { return dataManager_; };

    void applyParams(const ParamSet &p);

    enum Action {
        NONE,
        REAL_TIME_DETECTION,
        SINGLE_IMAGE_DETECTION
    };

    Action getCurrentAction() const { return curAction; }

    void stop();

    bool startRealTimeDetection();

    bool startSingleImageDetection(const string &imageName);

    int fetchAndClearFrameCounter();

    void reloadLists();

    int loadImageDataSet(const string &path);

private:

    Camera *camera_;
    ArmorDetector *detector_;
    ImageDataManager *dataManager_;

    ParamSet params;

    Action curAction = NONE;

    std::thread *th = nullptr;
    std::atomic<bool> threadShouldExit;
    std::atomic<int> frameCounter;

    void runRealTimeDetection();

};

}


#endif //META_VISION_SOLAIS_EXECUTOR_H
