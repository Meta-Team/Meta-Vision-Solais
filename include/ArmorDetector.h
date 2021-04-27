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

    const cv::Mat &getImgOriginal() const { return imgOriginal; };
    const cv::Mat &getImgGray() const { return imgGray; };
    const cv::Mat &getImgBrightnessThreshold() const { return imgBrightnessThreshold; };
    const cv::Mat &getImgColorThreshold() const { return imgColorThreshold; };
    const cv::Mat &getImgLights() const { return imgLights; };
    const cv::Mat &getImgArmors() const { return imgArmors; };

private:

    ParamSet params;

    cv::Mat imgOriginal;
    cv::Mat imgGray;
    cv::Mat imgBrightnessThreshold;
    cv::Mat imgColorThreshold;
    cv::Mat imgLights;

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
