//
// Created by liuzikai on 2/6/21.
//

#ifndef META_VISION_SOLAIS_ARMORDETECTOR_H
#define META_VISION_SOLAIS_ARMORDETECTOR_H

#include "Parameters.h"
#include <mutex>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace meta {

class ArmorDetector {
public:

    void setParams(const ParamSet &p) { params = p; }

    const ParamSet &getParams() const { return params; }

    std::vector<cv::Point2f> detect(const cv::Mat &img);

    void clearImages() { imgOriginal = imgGray = imgBrightness = imgColor = imgLights = imgArmors = cv::Mat(); }

    // Acquire the lock before accessing the images
    std::mutex outputMutex;
    const cv::Mat &originalImage() const { return imgOriginalOutput; };
    const cv::Mat &grayImage() const { return imgGrayOutput; };
    const cv::Mat &brightnessImage() const { return imgBrightnessOutput; };
    const cv::Mat &colorImage() const { return imgColorOutput; };
    const cv::Mat &contourImage() const { return imgContoursOutput; };
    const cv::Mat &armorImage() const { return imgArmorsOutput; };

private:

    ParamSet params;

    cv::Mat imgOriginal;
    cv::Mat imgGray;
    cv::Mat imgBrightness;
    cv::Mat imgColor;
    cv::Mat imgLights;
    cv::Mat imgContours;

    int acceptedContourCount;
    cv::Mat imgArmors;

    // Output Mats are assigned (no copying) after a completed detection so there is no intermediate result
    cv::Mat imgOriginalOutput;
    cv::Mat imgGrayOutput;
    cv::Mat imgBrightnessOutput;
    cv::Mat imgColorOutput;
    cv::Mat imgLightsOutput;
    cv::Mat imgContoursOutput;
    cv::Mat imgArmorsOutput;


    static void drawRotatedRect(cv::Mat &img, const cv::RotatedRect &rect, const cv::Scalar &boarderColor);

    /**
     * Canonicalize a non-square rotated rect from cv::minAreaRect and make:
     *  width: the short edge
     *  height: the long edge
     *  angle: in [0, 180). The angle is then the angle between the long edge and the vertical axis.
     *  https://stackoverflow.com/questions/22696539/reorder-four-points-of-a-rectangle-to-the-correct-order
     * @param rect
     */
    static void canonicalizeRotatedRect(cv::RotatedRect &rect);

};

}

#endif //META_VISION_SOLAIS_ARMORDETECTOR_H
