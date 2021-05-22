//
// Created by liuzikai on 5/22/21.
//

#ifndef META_VISION_SOLAIS_ARMORSOLVER_H
#define META_VISION_SOLAIS_ARMORSOLVER_H

#include <vector>
#include <array>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace meta {

class ArmorSolver {
public:

    /**
     * Initialize an ArmorSolver.
     * @param armorWidth  [mm]
     * @param armorHeight [mm]
     * @param cameraMatrix
     * @param distCoeffs
     * @param zScale
     */
    explicit ArmorSolver(float armorWidth, float armorHeight, const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs,
                         float zScale);

    /**
     * Solve armor position in physical world.
     * @param imagePoints
     * @return Displacement: x, y, z(distance) in mm
     */
    std::array<float, 3> solve(const std::array<cv::Point2f, 4> &imagePoints);

private:

    float armorWidth;
    float armorHeight;

    std::vector<cv::Point3f> armorObjectPoints;

    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;

    float zScale;
};

}

#endif //META_VISION_SOLAIS_ARMORSOLVER_H
