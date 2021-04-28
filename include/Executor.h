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

    const Camera *getCamera() const { return camera; }
    const ArmorDetector *getDetector() const { return detector; }
    const ImageDataManager *getDataManager() const { return dataManager; };

    void applyParams(const ParamSet &params);

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

    int loadDataSet(const string &path);

private:

    Camera *camera;
    ArmorDetector *detector;
    ImageDataManager *dataManager;

    ParamSet params;

    Action curAction = NONE;

    std::thread *th = nullptr;
    std::atomic<bool> threadShouldExit;
    std::atomic<int> frameCounter;

    void runRealTimeDetection();

};

}


#endif //META_VISION_SOLAIS_EXECUTOR_H
