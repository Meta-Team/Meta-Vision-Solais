//
// Created by liuzikai on 5/22/21.
//

#include "PositionCalculator.h"
#include <opencv2/calib3d.hpp>
#include <utility>

namespace meta {

void PositionCalculator::setParameters(cv::Point2f smallArmorSize_, cv::Point2f largeArmorSize_,
                                       const cv::Mat &cameraMatrix_, const cv::Mat &distCoeffs_, float zScale_) {
    smallArmorSize = std::move(smallArmorSize_);
    largeArmorSize = std::move(largeArmorSize_);
    cameraMatrix = cameraMatrix_;
    distCoeffs = distCoeffs_;
    zScale = zScale_;

    smallArmorObjectPoints = {{-smallArmorSize.x / 2, smallArmorSize.y / 2,  0},
                              {-smallArmorSize.x / 2, -smallArmorSize.y / 2, 0},
                              {smallArmorSize.x / 2,  -smallArmorSize.y / 2, 0},
                              {smallArmorSize.x / 2,  smallArmorSize.y / 2,  0}};

    largeArmorObjectPoints = {{-largeArmorSize.x / 2, largeArmorSize.y / 2,  0},
                              {-largeArmorSize.x / 2, -largeArmorSize.y / 2, 0},
                              {largeArmorSize.x / 2,  -largeArmorSize.y / 2, 0},
                              {largeArmorSize.x / 2,  largeArmorSize.y / 2,  0}};
    
    /*
     *              1 ----------- 2
     *            |*|             |*|
     * left light |*|     Orig    |*| right light
     *            |*|             |*|
     *              0 ----------- 3
     */

}

bool PositionCalculator::solve(const std::array<cv::Point2f, 4> &imagePoints, bool largeArmor,
                               cv::Point3f &offset, cv::Point3f &rotation) const {
    cv::Mat rVec = cv::Mat::zeros(3, 1, CV_64FC1);
    cv::Mat tVec = cv::Mat::zeros(3, 1, CV_64FC1);
    cv::solvePnP((largeArmor ? largeArmorObjectPoints : smallArmorObjectPoints), imagePoints,
                 cameraMatrix, distCoeffs, rVec, tVec, false, cv::SOLVEPNP_ITERATIVE);
    assert(tVec.rows == 3 && tVec.cols == 1);
    offset = {static_cast<float>(tVec.at<double>(0, 0)),
              static_cast<float>(tVec.at<double>(1, 0)),
              static_cast<float>(tVec.at<double>(2, 0)) * zScale};
    // FIXME: transform rotation matrix to yaw, pitch and roll
//    abaaba
    rotation = {static_cast<float>(rVec.at<double>(0, 0)),
                static_cast<float>(rVec.at<double>(1, 0)),
                static_cast<float>(rVec.at<double>(2, 0))};
    return true;
}

}