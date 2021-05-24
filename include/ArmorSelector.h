//
// Created by liuzikai on 5/22/21.
//

#ifndef META_VISION_SOLAIS_ARMORSELECTOR_H
#define META_VISION_SOLAIS_ARMORSELECTOR_H

#include <vector>
#include <array>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace meta {

class ArmorSelector {
public:

    struct ArmorInfo {
        std::array<cv::Point2f, 4> imgPoints;
        cv::Point2f imgCenter;
        cv::Point3f offsets;
    };

    ArmorInfo select(std::vector<ArmorInfo> armors);

private:

    struct DistanceLess {
        inline bool operator()(const ArmorInfo &a, const ArmorInfo &b) const {
            return cv::norm(a.offsets) < cv::norm(b.offsets);
        }
    };

};

}

#endif //META_VISION_SOLAIS_ARMORSELECTOR_H
