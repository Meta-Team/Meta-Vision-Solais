//
// Created by liuzikai on 4/18/21.
//

#ifndef META_VISION_SOLAIS_EXECUTOR_H
#define META_VISION_SOLAIS_EXECUTOR_H

#include "Common.h"
#include <thread>

namespace meta {

// Forward declarations
class Camera;
class ArmorDetector;

class Executor {
public:

    enum Action {
        NONE,
        REAL_TIME_DETECTION
    };

    explicit Executor(Camera *camera, ArmorDetector *detector);

    void setAction(Action action);

    Action getCurrentAction() const { return curAction; }

    int fetchAndClearFrameCounter();

private:

    Camera *camera;
    ArmorDetector *detector;

    Action curAction = NONE;

    std::thread *th = nullptr;
    std::atomic<bool> threadShouldExit;
    std::atomic<int> frameCounter;

    void runRealTimeDetection();

};

}


#endif //META_VISION_SOLAIS_EXECUTOR_H
