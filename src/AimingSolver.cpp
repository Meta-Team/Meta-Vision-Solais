//
// Created by liuzikai on 5/25/21.
//

#include "AimingSolver.h"
#include <algorithm>

namespace meta {

void AimingSolver::update(std::vector<ArmorInfo> armors) {
    if (!armors.empty()) {

        // Select the armor with minimal distance
        std::sort(armors.begin(), armors.end(), DistanceLess());

        auto &target = armors.front();

        latestCommand.yawDelta = -std::atan(target.offsets.x / target.offsets.z) * 180.0f / M_PI;
        latestCommand.pitchDelta = std::atan(target.offsets.y / target.offsets.z) * 180.0f / M_PI;
    } else {

        latestCommand.yawDelta = 0;
        latestCommand.pitchDelta = 0;

    }
}

AimingSolver::ControlCommand AimingSolver::getControlCommand() const {
    return latestCommand;
}

}