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
        bool largeArmor;
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
        float yawDelta = 0;
        float pitchDelta = 0;
        float distance = 0;
    };

    bool getControlCommand(ControlCommand &command) const;

private:

    package::ParamSet params;
    ControlCommand latestCommand;
    bool shouldSendCommand = false;

    bool tracking = false;
    int lost_tracking_frame_count = 0;
    ArmorInfo lastSelectedArmor;
    float lastDist = 0;

    static cv::Point3f xyzToYPD(const cv::Point3f &xyz);

    static constexpr float PI = 3.14159265358979323846264338327950288f;
    static constexpr float g = 9.81E3f;  // [mm/s^2]
};

}

#endif //META_VISION_SOLAIS_AIMINGSOLVER_H
