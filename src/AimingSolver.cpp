//
// Created by liuzikai on 5/25/21.
//

#include "AimingSolver.h"
#include <algorithm>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::microseconds;

namespace meta {

inline float pow2(float v) { return v * v; }

using std::sqrt;
using std::atan;
using cv::norm;

cv::Point3f AimingSolver::xyzToYPD(const cv::Point3f &xyz) {
    return {(float) (atan(xyz.x / xyz.z) * 180.0f / PI),  // +: horizontal right
            (float) (atan(xyz.y / sqrt(pow2(xyz.x) + pow2(xyz.z))) * 180.0f / PI),  // +: vertical down
            (float) norm(xyz)};  // +: away
}

void AimingSolver::updateArmors(std::vector<ArmorInfo> &armors, TimePoint imageCaptureTime) {

    frameCount++;
    ArmorInfo *selectedArmor = nullptr;

    if (armors.empty()) {

        topKiller.update(nullptr, imageCaptureTime);  // TopKiller needs to be updated before Tracker
        tracker.update(nullptr);

    } else {

        // Select the armor closest to the point required by the Tracker
        cv::Point2f targetImgPoint = tracker.getTargetImgPoint();
        float imgMinDist = 1000000;
        for (auto &armor : armors) {
            float imgDist = cv::norm(armor.imgCenter - targetImgPoint);
            if (imgDist < imgMinDist) {
                imgMinDist = imgDist;
                selectedArmor = &armor;
            }
        }

        // Update
        selectedArmor->ypd = xyzToYPD(selectedArmor->offset);
        selectedArmor->flags |= ArmorInfo::SELECTED_TARGET;
        topKiller.update(selectedArmor, imageCaptureTime);  // TopKiller needs to be updated before Tracker
        tracker.update(selectedArmor);

    }

    if (!topKiller.isTriggered()) {

        latestCommand.topKillerTriggered = false;

        if (selectedArmor) {

            // Aim at the selected armor
            const auto &xyz = selectedArmor->offset;
            const auto &ypd = selectedArmor->ypd;
            latestCommand.detected = true;
            latestCommand.yawDelta = ypd.x + params.manual_delta_offset().x();
            latestCommand.pitchDelta = ypd.y + params.manual_delta_offset().y();
            latestCommand.dist = ypd.z;
            latestCommand.remainingTimeToTarget = latestCommand.period = 0;
        } else {

            // Let Control keep last target angles
            latestCommand.detected = false;
        }

    } else {

        // Aim at the predicted target point

        latestCommand.topKillerTriggered = true;

        cv::Point3f ypd;
        latestCommand.detected = topKiller.shouldSendTarget(ypd);
        if (latestCommand.detected) {
            latestCommand.yawDelta = ypd.x + params.manual_delta_offset().x();
            latestCommand.pitchDelta = ypd.y + params.manual_delta_offset().y();
            /*
             * When an armor rotates to the target point, it will be little closer than the midpoint of two
             * rotated armors at the two sides.
             */
            latestCommand.dist = ypd.z + params.tk_target_dist_offset();
            latestCommand.remainingTimeToTarget = ((int) topKiller.getTimePointToTarget() - (int) imageCaptureTime) / 10;
            latestCommand.period = (int) topKiller.getPeriod() / 10;
        }

    }

    shouldSendCommand = true;  // always send the command for Control to update latest gimbal angles

}

/** Tracker **/

void AimingSolver::Tracker::update(const AimingSolver::ArmorInfo *selectedArmor) {
    if (selectedArmor == nullptr) {
        if (tracking) {
            lostArmorFrameCount++;
            if (lostArmorFrameCount > params.tracking_life_time()) {
                // Stop tracking and discard last selected armor
                tracking = false;
                lostArmorFrameCount = 0;
            }
        }
    } else {
        tracking = true;
        lostArmorFrameCount = 0;
        trackingArmor = *selectedArmor;
    }
}

cv::Point2f AimingSolver::Tracker::getTargetImgPoint() {
    if (tracking) {
        // Select the armor closest to the last selected armor in the image
        return trackingArmor.imgCenter;
    } else {
        // Select the armor closest to the center of the image
        return {params.roi_width() / 2.0f, params.roi_height() / 2.0f};
    }
}

void AimingSolver::Tracker::reset() {
    tracking = false;
    lostArmorFrameCount = 0;
}

/** TopKiller **/

void AimingSolver::setParams(const ParamSet &p) {
    params = p;
    resetHistory();
}

void AimingSolver::TopKiller::update(const AimingSolver::ArmorInfo *armor, TimePoint time) {

    targetUpdated = false;
    bool historyChanged = false;

    // Discard out-of-date pulse history
    while (!pulses.empty() && time - pulses.front().time > params.tk_threshold().y() * 10) {
        pulses.pop_front();
        historyChanged = true;
    }

    // Detect new pulses
    if (armor != nullptr && tracker.tracking) {
        const ArmorInfo *lastArmor = &tracker.trackingArmor;
        if (std::abs(armor->offset.y - lastArmor->offset.y) <= params.pulse_max_y_offset() &&
            std::abs(armor->offset.x - lastArmor->offset.x) >= params.pulse_min_x_offset()) {

            if (!pulses.empty() && time - pulses.back().time <= params.pulse_min_interval() * 10) {
                PulseInfo lastPulse = pulses.back();
                pulses.pop_back();
                pulses.emplace_back(PulseInfo{
                        lastPulse.ypdMid * 0.8f + ((lastArmor->ypd + armor->ypd) / 2) * 0.2f,
                        time
                });
            } else {
                pulses.emplace_back(PulseInfo{
                        (lastArmor->ypd + armor->ypd) / 2, time
                });
            }

            historyChanged = true;
        }
    }

    // Update state
    if (historyChanged) {

        if (pulses.size() >= params.tk_threshold().x()) {

            triggered = true;
            targetUpdated = true;

            const auto &lastPulse = pulses[pulses.size() - 1];

            // Compute target point and distance
            {
                // Only use last pulse since Vision is unaware of gimbal movement between two pulses
                targetYPD = lastPulse.ypdMid;
            }

            // Compute average period
            {
                auto count = std::min(pulses.size() - 1, (size_t) params.tk_compute_period_using_pulses());
                period = (lastPulse.time - pulses[pulses.size() - 1 - count].time) / count;
                timeToTarget = lastPulse.time + period / 2U;
            }

        } else {
            triggered = false;
        }

    }
}

bool AimingSolver::TopKiller::shouldSendTarget(cv::Point3f &targetYPD_) {
    if (targetUpdated) {
        targetYPD_ = targetYPD;
        return true;
    } else {
        return false;
    }
}

void AimingSolver::TopKiller::reset() {
    triggered = false;
}

void AimingSolver::resetHistory() {
    tracker.reset();
    topKiller.reset();
}

bool AimingSolver::getControlCommand(AimingSolver::ControlCommand &command) const {
    if (shouldSendCommand) {
        command = latestCommand;
        return true;
    } else {
        return false;
    }
}

}