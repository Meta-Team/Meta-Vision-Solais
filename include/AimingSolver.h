//
// Created by liuzikai on 5/25/21.
//

#ifndef META_VISION_SOLAIS_AIMINGSOLVER_H
#define META_VISION_SOLAIS_AIMINGSOLVER_H

#include <vector>
#include <array>
#include <chrono>
#include <mutex>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "Parameters.h"

namespace meta {

using TimePoint = std::chrono::steady_clock::time_point;

class AimingSolver {
public:

    struct ArmorInfo {
    public:
        ArmorInfo(const std::array<cv::Point2f, 4> &imgPoints, const cv::Point2f &imgCenter,
                  const cv::Point3f &offset, const cv::Point3f &rotation, bool largeArmor, int number = 0)
                : imgPoints(imgPoints), imgCenter(imgCenter), offset(offset), rotation(rotation),
                  largeArmor(largeArmor), number(number) {}

        std::array<cv::Point2f, 4> imgPoints;
        cv::Point2f imgCenter;
        cv::Point3f offset;
        cv::Point3f rotation;
        bool largeArmor;
        int number = 0;  // 0 for empty (no number sticker)

    private:
        unsigned flags = 0;
        TimePoint updateTime = TimePoint();

        friend class AimingSolver;
    };

    void setParams(const package::ParamSet &p) { params = p; }

    void resetHistory();

    void updateArmors(std::vector<ArmorInfo> armors, TimePoint imageCaptureTime);

    /**
     * Update gimbal's current angle based on reported angles and velocities from Control.
     * @param yawAngle       [degree]
     * @param pitchAngle     [degree]
     * @param yawVelocity    [degree/s]
     * @param pitchVelocity  [degree/s]
     * @note  This function can be called in another thread other than the Executor thread.
     */
    void updateGimbal(float yawAngle, float pitchAngle, float yawVelocity, float pitchVelocity);

    struct ControlCommand {

        enum ControlMode {
            RELATIVE_ANGLE,
            ABSOLUTE_ANGLE

        } mode;

        float yawDelta = 0;
        float pitchDelta = 0;
    };

    bool shouldSendControlCommand() const { return shouldSendCommand; }

    ControlCommand getControlCommand() const;

private:

    package::ParamSet params;
    ControlCommand latestCommand;
    bool shouldSendCommand = false;

    struct DistanceLess {
        inline bool operator()(const ArmorInfo &a, const ArmorInfo &b) const {
            return cv::norm(a.offset) < cv::norm(b.offset);
        }
    };

    TimePoint lastArmorUpdateTime = TimePoint();
    TimePoint lastGimbalUpdateTime = TimePoint();

    // Invalidate gimbal feedback and switch to relative angle mode after this time
    static constexpr int GIMBAL_UPDATE_LIFE_TIME = 100;  // [ms]

    enum class ArmorFlag : unsigned {
        NONE = 0,
        DISCARDED = 1,
    };

    float yawCurrentAngle = 0;
    float pitchCurrentAngle = 0;
    std::mutex gimbalAngleMutex;
};

}

#endif //META_VISION_SOLAIS_AIMINGSOLVER_H
