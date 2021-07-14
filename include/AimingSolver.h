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
#include "Utilities.h"

namespace meta {

class AimingSolver {
public:

    AimingSolver() : tracker(params), topKiller(params, tracker) {}

    struct ArmorInfo {
    public:
        ArmorInfo() = default;

        ArmorInfo(const std::array<cv::Point2f, 4> &imgPoints, const cv::Point2f &imgCenter,
                  const cv::Point3f &offset, bool largeArmor, int number = 0)
                : imgPoints(imgPoints), imgCenter(imgCenter), offset(offset),
                  largeArmor(largeArmor), number(number) {}

        std::array<cv::Point2f, 4> imgPoints;  // pixel
        cv::Point2f imgCenter;                 // pixel
        cv::Point3f offset;                    // x, y, z in mm, relative to current view
        bool largeArmor = false;
        int number = 0;                        // 0 for empty (no number sticker)

        cv::Point3f ypd;                       // YPD: Yaw (.x [deg]) + Pitch (.y [deg]) + Distance (.z [mm])

        enum Flag : unsigned {
            NONE = 0,
            SELECTED_TARGET = 1,
        };
        unsigned flags = 0;
    };

    void setParams(const package::ParamSet &p);

    void resetHistory();

    void updateArmors(std::vector<ArmorInfo> &armors, TimePoint imageCaptureTime);

    struct ControlCommand {
        bool detected;
        bool topKillerTriggered;
        float yawDelta = 0;         // rightward for positive [deg]
        float pitchDelta = 0;       // downward for positive [deg]
        float dist = 0;             // non-negative [mm]
        int remainingTimeToTarget;  // TopKiller: remaining time [ms] for the armor to reach the target, 0 otherwise
        int period;                 // TopKiller: rotation period [ms]
    };

    bool getControlCommand(ControlCommand &command) const;

    struct PulseInfo {
        cv::Point3f ypdMid;
        TimePoint startTime;
        TimePoint avgTime;
        TimePoint endTime;
        int frameCount = 0;
    };

private:

    package::ParamSet params;

    unsigned frameCount = 0;

    // Control command
    ControlCommand latestCommand;
    bool shouldSendCommand = false;

    // Tracker: select armor closest to the last target
    class Tracker {
    public:
        explicit Tracker(const ParamSet &params) : params(params) {}

        void update(const ArmorInfo *selectedArmor);

        cv::Point2f getTargetImgPoint();

        void reset();

        bool tracking = false;
        ArmorInfo trackingArmor;
        int lostArmorFrameCount = 0;

    private:
        const ParamSet &params;  // reference to AimingSolver's params

    } tracker;

    // TopKiller: killer for spinning tops
    class TopKiller {
    public:
        explicit TopKiller(const ParamSet &params, const Tracker &tracker) : params(params), tracker(tracker) {}

        void update(const ArmorInfo *armor, TimePoint time);  // called before Tracker's update

        bool isTriggered() const { return triggered; }

        TimePoint getTimePointToTarget() const { return timeToTarget; }

        TimePoint getPeriod() const { return period; }

        bool shouldSendTarget(cv::Point3f &targetYPD);

        void reset();

    private:
        const ParamSet &params;  // reference to AimingSolver's params
        const Tracker &tracker;  // reference to AimingSolver's Tracker

        std::deque<PulseInfo> pulses;

        bool triggered = false;
        bool targetUpdated = false;
        cv::Point3f targetYPD;
        TimePoint period;
        TimePoint timeToTarget;  // expected time when the tracking armor will reach the target point

        friend class Executor;

    } topKiller;


    // Helpers

    static cv::Point3f xyzToYPD(const cv::Point3f &xyz);

    static constexpr float PI = 3.14159265358979323846264338327950288f;

    friend class Executor;
};

}

#endif //META_VISION_SOLAIS_AIMINGSOLVER_H
