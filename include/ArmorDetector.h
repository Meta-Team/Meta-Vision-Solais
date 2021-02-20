//
// Created by liuzikai on 2/6/21.
//

#ifndef META_VISION_SOLAIS_ARMORDETECTOR_H
#define META_VISION_SOLAIS_ARMORDETECTOR_H

#include "Common.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace meta {

class ArmorDetector {
public:

    enum TargetColor {
        RED,
        BLUE
    };

    enum ColorThresholdMode {
        HSV,
        RB_CHANNELS
    };

    struct ParameterSet {
        TargetColor targetColor = BLUE;

        // ================================ Brightness Filter ================================

        double brightnessThreshold = 128;

        // ================================ Color Filter ================================

        ColorThresholdMode colorThresholdMode = RB_CHANNELS;
        Range<double> hsvRedHue = {150, 30}; // across the 0 (180) point
        Range<double> hsvBlueHue = {90, 150}; // across the 0 (180) point
        double rbChannelThreshold = 20;

        int colorDilate = 3;  // 0 for not enabled

        // ================================ Contour Filter ================================
        double contourRectMinSize = 5;  // 0 for not enabled
        double contourMinArea = 10;     // 0 for not enabled

        int longEdgeMinLength = 8;  // 0 for not enabled

        bool enableAspectRatioFilter = true;
        Range<double> aspectRatio = {10, 30};
    };

    void setParams(const ParameterSet &p) { params = p; }

    void detect(const cv::Mat &img);

private:

    ParameterSet params;

    cv::Mat imgOriginal;
    cv::Mat imgGray;
    cv::Mat imgBrightnessThreshold;
    cv::Mat imgColorThreshold;
    cv::Mat imgLights;
    cv::Mat imgContours;

    friend class MainWindow;

    static void drawRotatedRect(cv::Mat &img, const cv::RotatedRect& rect, const cv::Scalar& boarderColor);
};

}

#endif //META_VISION_SOLAIS_ARMORDETECTOR_H
