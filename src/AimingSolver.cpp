//
// Created by liuzikai on 5/25/21.
//

#include "AimingSolver.h"
#include <algorithm>

namespace meta {

void AimingSolver::updateArmors(std::vector<ArmorInfo> armors, TimePoint imageCaptureTime) {

    // Lock and fetch gimbal data to local variable
    float currentYaw, currentPitch;
    TimePoint gimbalUpdateTime;
    gimbalAngleMutex.lock();
    {
        currentYaw = yawCurrentAngle;
        currentPitch = pitchCurrentAngle;
        gimbalUpdateTime = lastGimbalUpdateTime;
    }
    gimbalAngleMutex.unlock();

    // Use RELATIVE_ANGLE mode if there is no gimbal feedback or it is out of date
    ControlCommand::ControlMode mode =
            (gimbalUpdateTime == TimePoint() ||
             std::chrono::duration_cast<std::chrono::milliseconds>(imageCaptureTime - gimbalUpdateTime).count() >
             GIMBAL_UPDATE_LIFE_TIME) ? ControlCommand::RELATIVE_ANGLE : ControlCommand::ABSOLUTE_ANGLE;

    if (mode != latestCommand.mode) {  // warn once on mode change
        std::cerr << "AimingSolver: switch to "
        << ((mode == ControlCommand::RELATIVE_ANGLE) ? "RELATIVE_ANGLE" : "ABSOLUTE_ANGLE")
        << " mode" << std::endl;
    }

    // TODO
    if (!armors.empty()) {

        // Select the armor with minimal distance
        std::sort(armors.begin(), armors.end(), DistanceLess());

        auto &target = armors.front();

        latestCommand.mode = ControlCommand::RELATIVE_ANGLE;

        latestCommand.yawDelta = -std::atan(target.offset.x / target.offset.z) * 180.0f / M_PI;
        if (params.yaw_delta_offset().enabled()) {
            latestCommand.yawDelta += params.yaw_delta_offset().val();
        }

        latestCommand.pitchDelta = std::atan(target.offset.y / target.offset.z) * 180.0f / M_PI;
        if (params.pitch_delta_offset().enabled()) {
            latestCommand.pitchDelta += params.pitch_delta_offset().val();
        }

        shouldSendCommand = true;

    } else {

        shouldSendCommand = false;

    }
}

void AimingSolver::resetHistory() {

}

void AimingSolver::updateGimbal(float yawAngle, float pitchAngle, float yawVelocity, float pitchVelocity) {
    gimbalAngleMutex.lock();
    {
        // Compensate feedback angle delay
        yawCurrentAngle = yawAngle + yawVelocity * params.gimbal_delay() / 1000;
        pitchCurrentAngle = pitchAngle + pitchVelocity * params.gimbal_delay() / 1000;
        lastGimbalUpdateTime = std::chrono::steady_clock::now();
    }
    gimbalAngleMutex.unlock();
}

AimingSolver::ControlCommand AimingSolver::getControlCommand() const {
    return latestCommand;
}

}