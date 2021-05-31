//
// Created by liuzikai on 5/25/21.
//

#ifndef META_VISION_SOLAIS_AIMINGSOLVER_H
#define META_VISION_SOLAIS_AIMINGSOLVER_H

#include <vector>
#include <array>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "Parameters.h"

namespace meta {

class AimingSolver {
public:

    struct ArmorInfo {
        std::array<cv::Point2f, 4> imgPoints;
        cv::Point2f imgCenter;
        cv::Point3f offsets;
    };

    void setParams(const package::ParamSet &p) { params = p; }

    void update(std::vector<ArmorInfo> armors);

    struct ControlCommand {
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
            return cv::norm(a.offsets) < cv::norm(b.offsets);
        }
    };
};

}

#endif //META_VISION_SOLAIS_AIMINGSOLVER_H
