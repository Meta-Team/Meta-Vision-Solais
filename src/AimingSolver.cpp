//
// Created by liuzikai on 5/25/21.
//

#include "AimingSolver.h"
#include <algorithm>

using std::chrono::duration_cast;
using std::chrono::microseconds;

namespace meta {

void AimingSolver::updateArmors(std::vector<DetectedArmorInfo> detectedArmors, TimePoint imageCaptureTime) {

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
    ControlMode newMode =
            (!params.enable_absolute_angle_mode() || gimbalUpdateTime == TimePoint() ||
             std::chrono::duration_cast<std::chrono::milliseconds>(imageCaptureTime - gimbalUpdateTime).count() >
             GIMBAL_UPDATE_LIFE_TIME) ? RELATIVE_ANGLE : ABSOLUTE_ANGLE;

    if (newMode != historyMode) {  // mode change
        std::cerr << "AimingSolver: switch to "
                  << (newMode == RELATIVE_ANGLE ? "RELATIVE_ANGLE" : "ABSOLUTE_ANGLE")
                  << " mode" << std::endl;

        resetHistory();
        historyMode = newMode;
    }

    ArmorHistory *targetH = nullptr;  // a place holder

    if (!detectedArmors.empty()) {

        // First, choose the armor with smallest relative distance, and set targetArmor as processing
        {
            DetectedArmorInfo *targetD = nullptr;
            float minDist = 1000000;
            for (auto &d : detectedArmors) {
                float dist = cv::norm(d.offset);
                if (dist < minDist) {
                    targetD = &d;
                    dist = minDist;
                }
            }
            targetD->flags &= DetectedArmorInfo::SELECTED_TARGET;
        }

        // Calculate positions
        for (auto &d : detectedArmors) {
            d.ypd = {(float) (-std::atanf(d.offset.x / d.offset.z) * 180.0f / M_PI),  // minus sign
                     (float) (std::atanf(d.offset.y / d.offset.z) * 180.0f / M_PI),
                     (float) cv::norm(d.offset)};
            if (historyMode == ABSOLUTE_ANGLE) {  // use absolute angles if is in ABSOLUTE_ANGLE mode
                d.ypd.yaw = d.ypd.yaw + yawCurrentAngle;
                d.ypd.pitch = d.ypd.pitch + pitchCurrentAngle;

                d.xyz.x = std::sinf(d.ypd.yaw * M_PI / 180.0f) * d.ypd.dist;
                d.xyz.y = std::sinf(d.ypd.pitch * M_PI / 180.0f) * d.ypd.dist;
                d.xyz.z = std::sqrt(
                        d.ypd.dist * d.ypd.dist - d.xyz.x * d.xyz.x - d.xyz.y * d.xyz.y);
            } else {
                d.xyz = d.offset;
            }

        }

        // Update history with detected armors
        // The matching function skipped the processed detected armors, so the order may matter a little, but anyway...
        for (auto &h : history) {

            DetectedArmorInfo *d = matchArmor(detectedArmors, h, imageCaptureTime);

            if (d) {  // matched
                h.xyz.emplace_front(d->xyz);
                h.ypd.emplace_front(d->ypd);
                h.time.emplace_front(imageCaptureTime);

                // Preserve K + 1 positions to calculate K velocity
                if (h.count() > params.predict_backward_count() + 1) {
                    h.xyz.pop_back();
                    h.ypd.pop_back();
                    h.time.pop_back();
                }

                d->flags &= DetectedArmorInfo::PROCESSED;

                if (d->flags & DetectedArmorInfo::SELECTED_TARGET) targetH = &h;
            }
        }

    }

    // Discard lost armors
    for (auto it = history.cbegin(); it != history.cend(); /* no increment */) {
        if (!it->time.empty() &&
            imageCaptureTime - it->time.front() > std::chrono::milliseconds(params.armor_life_time())) {

            it = history.erase(it);
        } else {
            ++it;
        }
    }

    // Add new armors
    for (auto &d : detectedArmors) {
        if (!(d.flags & DetectedArmorInfo::PROCESSED)) {  // not matched with history
            auto &h = history.emplace_front(ArmorHistory{nextArmorIndex++, d.xyz, d.ypd, imageCaptureTime});
            d.flags &= DetectedArmorInfo::PROCESSED;
            if (d.flags & DetectedArmorInfo::SELECTED_TARGET) targetH = &h;
        }
    }

    if (targetH) {

        latestCommand.mode = historyMode;

        float dist = targetH->ypd.front().dist;

        // Aiming at the target
        {
            latestCommand.yaw = targetH->ypd.front().yaw;
            latestCommand.pitch = targetH->ypd.front().pitch;
        }

        // Compensate for moving speed
        if (targetH->count() > 1){
            float remainingWeight = 1;
            float historyWeight = params.predict_backward_fraction();

            YPD velocity;

            // Apply predict_backward_fraction starting from the second velocity record
            for (int i = 1; i < targetH->count() - 1; i++) {
                YPD v = targetH->ypd[i] - targetH->ypd[i + 1];
                velocity += v * historyWeight;
                remainingWeight -= historyWeight;
                historyWeight *= params.predict_backward_fraction();
            }

            if (remainingWeight <= 0) {
                std::cerr << "AimingSolver: sum of weights of history velocities exceeded 1" << std::endl;
                shouldSendCommand = false;
                return;
            }

            // Use the remaining weight for the first velocity
            velocity += (targetH->ypd[0] - targetH->ypd[1]) * remainingWeight;

            // Compensate
            velocity *= ((float) duration_cast<microseconds>(imageCaptureTime - targetH->time.front()).count()) / 1E6;
            latestCommand.yaw += velocity.yaw;
            latestCommand.pitch += velocity.pitch;
            dist += velocity.dist;

        }  // otherwise, no velocity available as there is only one position record

        // Compensate for distance
        // TODO:

        // Manual offsets
        if (params.yaw_delta_offset().enabled()) {
            latestCommand.yaw += params.yaw_delta_offset().val();
        }
        if (params.pitch_delta_offset().enabled()) {
            latestCommand.pitch += params.pitch_delta_offset().val();
        }

        shouldSendCommand = true;

    } else {
        shouldSendCommand = false;
    }

}

AimingSolver::DetectedArmorInfo *AimingSolver::matchArmor(std::vector<DetectedArmorInfo> &candidates,
                                                          const ArmorHistory &target,
                                                          const TimePoint &time) const {
    DetectedArmorInfo *ret = nullptr;
    float minDist = 1000000;
    for (auto &candidate : candidates) {
        if (candidate.flags & DetectedArmorInfo::PROCESSED) continue;
        if (!target.xyz.empty()) {
            float dist = cv::norm(candidate.xyz - target.xyz.front());  // mm
            float velocity =
                    dist /
                    std::chrono::duration_cast<std::chrono::microseconds>(time - target.time.front()).count() *
                    1E6;  // mm/s
            if (velocity <= params.tracking_max_velocity()) {
                if (dist < minDist) {
                    ret = &candidate;
                    minDist = dist;
                }
            }
        }
    }
    return ret;
}

void AimingSolver::resetHistory() {
    history.clear();
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