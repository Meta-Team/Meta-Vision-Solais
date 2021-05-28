//
// Created by liuzikai on 5/22/21.
//

#include "PositionCalculator.h"
#include <opencv2/calib3d.hpp>

namespace meta {

void PositionCalculator::setParameters(float armorWidth_, float armorHeight_, const cv::Mat &cameraMatrix_,
                                       const cv::Mat &distCoeffs_, float zScale_) {
        armorWidth = armorWidth_;
        armorHeight = armorHeight_;
        cameraMatrix = cameraMatrix_;
        distCoeffs = distCoeffs_;
        zScale = zScale_;

        armorObjectPoints = {{-armorWidth / 2, armorHeight / 2,  0},
                             {-armorWidth / 2, -armorHeight / 2, 0},
                             {armorWidth / 2,  -armorHeight / 2, 0},
                             {armorWidth / 2,  armorHeight / 2,  0}};
        /*
         *              1 ----------- 2
         *            |*|             |*|
         * left light |*|     Orig    |*| right light
         *            |*|             |*|
         *              0 ----------- 3
         */

}

bool PositionCalculator::solve(const std::array<cv::Point2f, 4> &imagePoints, cv::Point3f &offset) const {
    cv::Mat rVec = cv::Mat::zeros(3, 1, CV_64FC1);
    cv::Mat tVec = cv::Mat::zeros(3, 1, CV_64FC1);
    cv::solvePnP(armorObjectPoints, imagePoints, cameraMatrix, distCoeffs, rVec, tVec, false, cv::SOLVEPNP_ITERATIVE);
    assert(tVec.rows == 3 && tVec.cols == 1);
    offset = {static_cast<float>(tVec.at<double>(0, 0)),
              static_cast<float>(tVec.at<double>(1, 0)),
              static_cast<float>(tVec.at<double>(2, 0)) * zScale};
    return true;
}

}