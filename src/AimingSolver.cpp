//
// Created by liuzikai on 5/25/21.
//

#include "AimingSolver.h"
#include <algorithm>
#include <math.h>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::microseconds;

namespace meta {

inline float pow2(float v) { return v * v; }

AimingSolver::YPD AimingSolver::xyzToYPD(const AimingSolver::XYZ &xyz) {
    return {(float) (std::atan(xyz.x / xyz.z) * 180.0f / PI),   // horizontal right
            (float) (std::atan(xyz.y / xyz.z) * 180.0f / PI),  // vertical down
            (float) cv::norm(xyz)};                                    // away
}

cv::Size2f AimingSolver::imgPointsToSize(const std::array<cv::Point2f, 4> &imgPoints) {
    return {((imgPoints[2].x - imgPoints[1].x) + (imgPoints[3].x - imgPoints[0].x)) / 2,
            ((imgPoints[0].y - imgPoints[1].y) + (imgPoints[3].y - imgPoints[2].y)) / 2};
}

void AimingSolver::updateArmors(std::vector<DetectedArmorInfo> &detectedArmors, TimePoint imageCaptureTime) {

    // Lock and fetch gimbal data to local variables
    TimePoint gimbalUpdateTime;
    gimbalAngleMutex.lock();
    {
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

    ArmorHistory *targetH = nullptr;  // a place holder, history is std::list so pointers are valid

    if (!detectedArmors.empty()) {

        // First, choose the armor with smallest relative distance, and set targetH as processing
        {
            DetectedArmorInfo *targetD = nullptr;
            float minDist = 1000000;
            for (auto &d : detectedArmors) {
                float dist = cv::norm(d.offset);
                if (dist < minDist) {
                    targetD = &d;
                    minDist = dist;
                }
            }
            targetD->flags |= DetectedArmorInfo::SELECTED_TARGET;
        }

        // Fetch and compensate current gimbal angles into local variables
        float currentYaw, currentPitch;
        gimbalAngleMutex.lock();
        {
            currentYaw = yawCurrentAngle;
            currentPitch = pitchCurrentAngle;

            float timeOffset = duration_cast<microseconds>(imageCaptureTime - gimbalUpdateTime).count() / 1E6f;
            currentYaw += yawCurrentVelocity * timeOffset;
            currentPitch += pitchCurrentVelocity * timeOffset;
        }
        gimbalAngleMutex.unlock();

        // Calculate positions
        for (auto &d : detectedArmors) {
            d.ypd = xyzToYPD(d.offset);
            if (historyMode == ABSOLUTE_ANGLE) {  // use absolute angles if is in ABSOLUTE_ANGLE mode
                d.ypd.yaw = d.ypd.yaw + currentYaw;
                d.ypd.pitch = d.ypd.pitch + currentPitch;

                d.xyz.x = std::sin(d.ypd.yaw * PI / 180.0f) * d.ypd.dist;
                d.xyz.y = std::sin(d.ypd.pitch * PI / 180.0f) * d.ypd.dist;
                d.xyz.z = std::sqrt(pow2(d.ypd.dist) - pow2(d.xyz.x) - pow2(d.xyz.y));
            } else {
                d.xyz = d.offset;
            }
        }

        // Update history with detected armors
        // The matching function skipped the processed detected armors, so the order may matter a little, but anyway...
        for (auto &h : history) {

            DetectedArmorInfo *d = matchArmor(detectedArmors, h, imageCaptureTime);

            if (d) {  // matched
                h.imgCenter.emplace_front(d->imgCenter);
                h.imgSize.emplace_back(imgPointsToSize(d->imgPoints));
                h.xyz.emplace_front(d->xyz);
                h.ypd.emplace_front(d->ypd);
                h.time.emplace_front(imageCaptureTime);

                // Preserve K + 1 positions to calculate K velocity
                if (h.count() > params.predict_backward_count() + 1) {
                    h.imgCenter.pop_back();
                    h.imgSize.pop_back();
                    h.xyz.pop_back();
                    h.ypd.pop_back();
                    h.time.pop_back();
                }

                d->history_index = h.index;
                d->flags |= DetectedArmorInfo::PROCESSED;

                if (d->flags & DetectedArmorInfo::SELECTED_TARGET) targetH = &h;
            }
        }

    }

    // Discard lost armors
    for (auto it = history.cbegin(); it != history.cend(); /* no increment */) {
        if (imageCaptureTime - it->time.front() > milliseconds(params.armor_life_time())) {
            it = history.erase(it);
            if (targetH && targetH->index == it->index) {
                targetH = nullptr;
            }
        } else {
            ++it;
        }
    }

    // Add new armors
    for (auto &d : detectedArmors) {
        if (!(d.flags & DetectedArmorInfo::PROCESSED)) {  // not matched with history
            auto &h = history.emplace_front(ArmorHistory{nextArmorIndex++, d.imgCenter, imgPointsToSize(d.imgPoints),
                                                         d.xyz, d.ypd, imageCaptureTime});
            d.history_index = h.index;
            d.flags |= DetectedArmorInfo::PROCESSED;
            if (d.flags & DetectedArmorInfo::SELECTED_TARGET) targetH = &h;
        }
    }

    if (targetH) {

        // Position
        XYZ xyz = targetH->xyz.front();

        // Compensate for moving speed and control command delay
        if (targetH->count() > 1 && params.predict_backward_count() >= 1) {
            float remainingWeight = 1;
            float historyWeight = params.predict_backward_fraction();

            XYZ velocity = {0, 0, 0};

            // Apply predict_backward_fraction starting from the second velocity record
            for (int i = 1; i < std::min(targetH->count(), (unsigned long) params.predict_backward_count()) - 1; i++) {
                XYZ v = targetH->xyz[i] - targetH->xyz[i + 1];
                velocity += v * historyWeight;
                remainingWeight -= historyWeight;
                historyWeight *= params.predict_backward_fraction();
            }

            if (remainingWeight < 0) {
                std::cerr << "AimingSolver: sum of weights of history velocities exceeded 1" << std::endl;
                shouldSendCommand = false;
                return;
            }

            // Use the remaining weight for the first velocity
            velocity += (targetH->xyz[0] - targetH->xyz[1]) * remainingWeight;

            // Compensate
            float time = ((float) duration_cast<microseconds>(imageCaptureTime - targetH->time.front()).count()) / 1E6 +
                         params.control_command_delay() / 1E3;  // [s]
            xyz += velocity * time;

        }  // otherwise, no velocity available as there is only one position record

        // Aiming at the expected position
        YPD aiming = xyzToYPD(xyz);

        // Compensate for distance
        if (params.compensate_bullet_speed().enabled()) {
            aiming.pitch = std::atan(
                    (pow2(bulletSpeed) / g - std::sqrt(pow2(pow2(bulletSpeed) / g - xyz.y) - pow2(aiming.dist))) /
                    std::sqrt(pow2(xyz.z) + pow2(xyz.x))
            ) / PI * 180;
        }

        // Manual offsets
        if (params.yaw_delta_offset().enabled()) {
            aiming.yaw += params.yaw_delta_offset().val();
        }
        if (params.pitch_delta_offset().enabled()) {
            aiming.pitch += params.pitch_delta_offset().val();
        }

        // Set control command
        latestCommand.mode = historyMode;
        latestCommand.yaw = aiming.yaw;
        latestCommand.pitch = aiming.pitch;
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

        if (params.armor_tracking_mode() == package::ParamSet_ArmorTrackingMode_IMAGE) {

            // IMAGE tracking mode
            cv::Point2f offset = candidate.imgCenter - target.imgCenter.front();
            if (std::abs(offset.x) <= target.imgSize.front().width * params.image_max_offset_fraction() &&
                std::abs(offset.y) <= target.imgSize.front().height * params.image_max_offset_fraction()) {

                float dist = cv::norm(offset);
                if (dist < minDist) {
                    ret = &candidate;
                    minDist = dist;
                }
            }

        } else {

            // PHYSICAL tracking mode

            float dist = cv::norm(candidate.xyz - target.xyz.front());  // [mm]
            float velocity = dist * 1E6f / duration_cast<microseconds>(time - target.time.front()).count();  // [mm/s]
            if (velocity <= params.physical_max_velocity()) {
                if (dist < minDist) {
                    ret = &candidate;
                    minDist = dist;
                }
            }
        }

    }
    return ret;
}

void AimingSolver::setParams(const ParamSet &p) {
    params = p;
    bulletSpeed = p.compensate_bullet_speed().val();
    resetHistory();
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