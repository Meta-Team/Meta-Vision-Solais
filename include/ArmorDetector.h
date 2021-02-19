//
// Created by liuzikai on 2/6/21.
//

#ifndef META_VISION_SOLAIS_ARMORDETECTOR_H
#define META_VISION_SOLAIS_ARMORDETECTOR_H

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

        float brightnessThreshold = 128;

        ColorThresholdMode colorThresholdMode = RB_CHANNELS;
        float hsvRedMin = 150, hsvRedMax = 30;  // across the 0 (180) point
        float hsvBlueMin = 90, hsvBlueMax = 150;
        float rbChannelThreshold = 20;
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
