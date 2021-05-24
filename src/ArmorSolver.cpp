//
// Created by liuzikai on 5/22/21.
//

#include "ArmorSolver.h"
#include <opencv2/calib3d.hpp>

namespace meta {

ArmorSolver::ArmorSolver(float armorWidth, float armorHeight, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs,
                         float zScale)
        : armorWidth(armorWidth), armorHeight(armorHeight),
          cameraMatrix(cameraMatrix.clone()), distCoeffs(distCoeffs.clone()),
          armorObjectPoints({
                                    {-armorWidth / 2, armorHeight / 2,  0},
                                    {-armorWidth / 2, -armorHeight / 2, 0},
                                    {armorWidth / 2,  -armorHeight / 2, 0},
                                    {armorWidth / 2,  armorHeight / 2,  0}
                            }), zScale(zScale) {
    /*
     *              1 ----------- 2
     *            |*|             |*|
     * left light |*|     Orig    |*| right light
     *            |*|             |*|
     *              0 ----------- 3
     */

}

cv::Point3f ArmorSolver::solve(const std::array<cv::Point2f, 4> &imagePoints) {
    cv::Mat rVec = cv::Mat::zeros(3, 1, CV_64FC1);
    cv::Mat tVec = cv::Mat::zeros(3, 1, CV_64FC1);
    cv::solvePnP(armorObjectPoints, imagePoints, cameraMatrix, distCoeffs, rVec, tVec, false, cv::SOLVEPNP_ITERATIVE);
    assert(tVec.rows == 3 && tVec.cols == 1);
    return {static_cast<float>(tVec.at<double>(0, 0)),
            static_cast<float>(tVec.at<double>(1, 0)),
            static_cast<float>(tVec.at<double>(2, 0)) * zScale};
}

}