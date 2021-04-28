//
// Created by liuzikai on 2/6/21.
//

#ifndef META_VISION_SOLAIS_ARMORDETECTOR_H
#define META_VISION_SOLAIS_ARMORDETECTOR_H

#include "Common.h"
#include "Parameters.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace meta {

class ArmorDetector {
public:

    void setParams(const ParamSet &p) { params = p; }

    const ParamSet &getParams() const { return params; }

    vector<cv::Point2f> detect(const cv::Mat &img);

    void clearImages() { imgOriginal = imgGray = imgBrightness = imgColor = imgLights = imgArmors = cv::Mat(); }
    const cv::Mat &originalImage() const { return imgOriginal; };
    const cv::Mat &grayImage() const { return imgGray; };
    const cv::Mat &brightnessImage() const { return imgBrightness; };
    const cv::Mat &colorImage() const { return imgColor; };
    const cv::Mat &contourImage() const { return imgContours; };
    const cv::Mat &armorImage() const { return imgArmors; };

private:

    ParamSet params;

    cv::Mat imgOriginal;
    cv::Mat imgGray;
    cv::Mat imgBrightness;
    cv::Mat imgColor;
    cv::Mat imgLights;
    cv::Mat imgContours;  // only for debug

    int acceptedContourCount;
    cv::Mat imgArmors;

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
