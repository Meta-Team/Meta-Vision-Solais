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

    if (armors.empty()) {

        if (tracking) {
            lost_tracking_frame_count++;
            if (lost_tracking_frame_count > params.tracking_life_time()) {
                // Stop tracking and discard last selected armor
                tracking = false;
                lost_tracking_frame_count = 0;
            }
        }

        latestCommand.detected = false;
        shouldSendCommand = true;

    } else {

        lost_tracking_frame_count = 0;

        cv::Point2f targetImgPoint;
        if (tracking) {
            // Select the armor closest to the last selected armor in the image
            targetImgPoint = lastSelectedArmor.imgCenter;
        } else {
            // Select the armor closest to the center of the image
            targetImgPoint = {params.roi_width() / 2.0f, params.roi_height() / 2.0f};
        }

        float imgMinDist = 1000000;
        ArmorInfo *selectedArmor = nullptr;
        for (auto &armor : armors) {
            float imgDist = cv::norm(armor.imgCenter - targetImgPoint);
            if (imgDist < imgMinDist) {
                imgMinDist = imgDist;
                selectedArmor = &armor;
            }
        }

        selectedArmor->ypd = xyzToYPD(selectedArmor->offset);
        selectedArmor->flags |= ArmorInfo::SELECTED_TARGET;
        lastSelectedArmor = *selectedArmor;

        const auto &xyz = selectedArmor->offset;
        const auto &ypd = selectedArmor->ypd;

        if (!tracking) {
            tracking = true;
            lastDist = ypd.z;
        }

        float targetYaw = ypd.x;
        float targetPitch = ypd.y;

        lastDist = ypd.z * 0.1 + lastDist * 0.9;

        if (params.compensate_bullet_speed().enabled()) {
            // Compensate gravity
            float v0 = params.compensate_bullet_speed().val();  // [mm/s]
            float a = pow2(v0) / g;
            targetPitch = -atan((a - sqrt(pow2(a - (-xyz.y)) - pow2(lastDist))) / sqrt(pow2(xyz.z) + pow2(xyz.x))) * 180.0f / PI;
        }

        // Manual offset
        targetYaw += params.manual_delta_offset().x();
        targetPitch += params.manual_delta_offset().y();

        latestCommand.detected = true;
        latestCommand.yawDelta = targetYaw;
        latestCommand.pitchDelta = targetPitch;
        latestCommand.distance = lastDist;
        shouldSendCommand = true;
    }

}

void AimingSolver::setParams(const ParamSet &p) {
    params = p;
    resetHistory();
}

void AimingSolver::resetHistory() {
    tracking = false;
    lost_tracking_frame_count = 0;
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