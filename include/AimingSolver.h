//
// Created by liuzikai on 5/25/21.
//

#ifndef META_VISION_SOLAIS_AIMINGSOLVER_H
#define META_VISION_SOLAIS_AIMINGSOLVER_H

#include <vector>
#include <array>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace meta {

class AimingSolver {
public:

    struct ArmorInfo {
        std::array<cv::Point2f, 4> imgPoints;
        cv::Point2f imgCenter;
        cv::Point3f offsets;
    };

    void update(std::vector<ArmorInfo> armors);

    struct ControlCommand {
        float yawDelta = 0;
        float pitchDelta = 0;
    };

    ControlCommand getControlCommand() const;

private:

    ControlCommand latestCommand;

    struct DistanceLess {
        inline bool operator()(const ArmorInfo &a, const ArmorInfo &b) const {
            return cv::norm(a.offsets) < cv::norm(b.offsets);
        }
    };
};

}

#endif //META_VISION_SOLAIS_AIMINGSOLVER_H
