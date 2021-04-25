//
// Created by liuzikai on 4/18/21.
//

#ifndef META_VISION_SOLAIS_EXECUTOR_H
#define META_VISION_SOLAIS_EXECUTOR_H

#include "Common.h"

namespace meta {

class Camera;

class Executor {
public:

    explicit Executor(Camera *camera);



private:

    Camera *camera;

};

}


#endif //META_VISION_SOLAIS_EXECUTOR_H
