//
// Created by liuzikai on 5/22/21.
//

#ifndef META_VISION_SOLAIS_POSITIONCALCULATOR_H
#define META_VISION_SOLAIS_POSITIONCALCULATOR_H

#include <vector>
#include <array>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace meta {

class PositionCalculator {
public:

    /**
     *
     * @param smallArmorSize  [mm]
     * @param largeArmorSize  [mm]
     * @param cameraMatrix
     * @param distCoeffs
     * @param zScale
     */
    void setParameters(cv::Point2f smallArmorSize, cv::Point2f largeArmorSize,
                       const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs, float zScale);

    /**
     * Solve armor position in physical world.
     * @param imagePoints
     * @param largeArmor
     * @param offset       Displacement: x, y, z(distance) in mm
     * @return
     */
    bool solve(const std::array<cv::Point2f, 4> &imagePoints, bool largeArmor, cv::Point3f &offset) const;

private:

    cv::Point2f smallArmorSize;
    cv::Point2f largeArmorSize;

    std::vector<cv::Point3f> smallArmorObjectPoints;
    std::vector<cv::Point3f> largeArmorObjectPoints;

    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;

    float zScale;
};

}

#endif //META_VISION_SOLAIS_POSITIONCALCULATOR_H
