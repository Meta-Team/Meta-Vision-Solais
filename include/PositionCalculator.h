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
     * @param armorWidth  [mm]
     * @param armorHeight [mm]
     * @param cameraMatrix
     * @param distCoeffs
     * @param zScale
     */
    void setParameters(float armorWidth, float armorHeight, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs,
                       float zScale);

    /**
     * Solve armor position in physical world.
     * @param imagePoints
     * @param offset       Displacement: x, y, z(distance) in mm
     * @return
     */
    bool solve(const std::array<cv::Point2f, 4> &imagePoints, cv::Point3f &offset) const;

private:

    float armorWidth = 0;
    float armorHeight = 0;

    std::vector<cv::Point3f> armorObjectPoints;

    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;

    float zScale;
};

}

#endif //META_VISION_SOLAIS_POSITIONCALCULATOR_H
