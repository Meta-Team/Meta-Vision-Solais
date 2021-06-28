//
// Created by liuzikai on 5/25/21.
//

#ifndef META_VISION_SOLAIS_AIMINGSOLVER_H
#define META_VISION_SOLAIS_AIMINGSOLVER_H

#include <vector>
#include <array>
#include <list>
#include <deque>
#include <chrono>
#include <mutex>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "Parameters.h"

namespace meta {

using TimePoint = std::chrono::steady_clock::time_point;

class AimingSolver {
private:

    using XYZ = cv::Point3f;

    struct YPD {          // yaw, pitch, distance coordinate
        float yaw = 0;    // [degree]
        float pitch = 0;  // [degree]
        float dist = 0;   // [mm]

        YPD operator- (const YPD &other) const {
            return {yaw - other.yaw, pitch - other.pitch, dist - other.dist};
        }

        YPD operator+ (const YPD &other) const {
            return {yaw + other.yaw, pitch + other.pitch, dist + other.dist};
        }

        YPD operator+= (const YPD &other)  {
            return {yaw += other.yaw, pitch += other.pitch, dist += other.dist};
        }

        YPD operator* (float f) const {
            return {yaw * f, pitch * f, dist * f};
        }

        YPD operator*= (float f)  {
            return {yaw *= f, pitch *= f, dist *= f};
        }
    };

public:

    struct DetectedArmorInfo {
    public:
        DetectedArmorInfo(const std::array<cv::Point2f, 4> &imgPoints, const cv::Point2f &imgCenter,
                          const cv::Point3f &offset, const cv::Point3f &rotation, bool largeArmor, int number = 0)
                : imgPoints(imgPoints), imgCenter(imgCenter), offset(offset), rotation(rotation),
                  largeArmor(largeArmor), number(number) {}

        std::array<cv::Point2f, 4> imgPoints;  // pixel
        cv::Point2f imgCenter;                 // pixel
        cv::Point3f offset;                    // x, y, z in mm, relative to current view
        cv::Point3f rotation;                  // unused yet
        bool largeArmor;
        int number = 0;                        // 0 for empty (no number sticker)

    private:

        enum Flag : unsigned {
            NONE = 0,
            PROCESSED = 1,
            SELECTED_TARGET = 2,
        };
        unsigned flags = 0;

        XYZ xyz;  // XYZ coordinate in relative (the same as offset) or actual world
        YPD ypd;  // YPD position in relative or actual world

        friend class AimingSolver;
    };

    void setParams(const package::ParamSet &p) {
        params = p;
        resetHistory();
    }

    void resetHistory();

    void updateArmors(std::vector<DetectedArmorInfo> detectedArmors, TimePoint imageCaptureTime);

    /**
     * Update gimbal's current angle based on reported angles and velocities from Control.
     * @param yawAngle       [degree]
     * @param pitchAngle     [degree]
     * @param yawVelocity    [degree/s]
     * @param pitchVelocity  [degree/s]
     * @note  This function can be called in another thread other than the Executor thread.
     */
    void updateGimbal(float yawAngle, float pitchAngle, float yawVelocity, float pitchVelocity);

    enum ControlMode {
        RELATIVE_ANGLE,
        ABSOLUTE_ANGLE
    };

    struct ControlCommand {
        ControlMode mode;

        float yaw = 0;
        float pitch = 0;
    };

    bool shouldSendControlCommand() const { return shouldSendCommand; }

    ControlCommand getControlCommand() const;

private:

    package::ParamSet params;
    ControlCommand latestCommand;
    bool shouldSendCommand = false;

    struct DistanceLess {
        inline bool operator()(const DetectedArmorInfo &a, const DetectedArmorInfo &b) const {
            return cv::norm(a.offset) < cv::norm(b.offset);
        }
    };

    TimePoint lastArmorUpdateTime = TimePoint();
    TimePoint lastGimbalUpdateTime = TimePoint();

    // Invalidate gimbal feedback and switch to relative angle mode after this time
    static constexpr int GIMBAL_UPDATE_LIFE_TIME = 100;  // [ms]

    float yawCurrentAngle = 0;
    float pitchCurrentAngle = 0;
    std::mutex gimbalAngleMutex;

    struct ArmorHistory {

        ArmorHistory(unsigned index, const XYZ &initXYZ, const YPD &initYPD, const TimePoint &initTime)
        : index(index), xyz({initXYZ}), ypd({initYPD}), time({initTime}) {};

        unsigned index;

        std::deque<XYZ> xyz;
        std::deque<YPD> ypd;
        std::list<TimePoint> time;

        auto count() const { return time.size(); }

        enum Flag : unsigned {
            NONE = 0,
        };
        unsigned flags = 0;
    };

    unsigned nextArmorIndex = 0;

    ControlMode historyMode;
    std::deque<ArmorHistory> history;

    DetectedArmorInfo *matchArmor(std::vector<DetectedArmorInfo> &candidates, const ArmorHistory &target,
                                         const TimePoint &time) const;
};

}

#endif //META_VISION_SOLAIS_AIMINGSOLVER_H
