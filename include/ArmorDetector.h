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

    enum ContourFitFunction {
        MIN_AREA_RECT,
        ELLIPSE
    };

    struct ParameterSet {

        /*
         * Note: int/double parameters should be preserved even when the filter is not enabled. So keep a bool value
         * for enabled rather than using special values to represent not enabled.
         */

        TargetColor targetColor = BLUE;

        // ================================ Brightness Filter ================================

        double brightnessThreshold = 155;

        // ================================ Color Filter ================================

        ColorThresholdMode colorThresholdMode = RB_CHANNELS;
        Range<double> hsvRedHue = {150, 30};  // across the 0 (180) point
        Range<double> hsvBlueHue = {90, 150};
        double rbChannelThreshold = 55;

        bool enableColorDilate = true;
        int colorDilate = 6;

        // ================================ Contour Filter ================================

        ContourFitFunction contourFitFunction = ELLIPSE;

        bool filterContourPixelCount = true;
        double contourPixelCount = 15;

        bool filterContourMinArea = false;
        double contourMinArea = 3;

        bool filterLongEdgeMinLength = true;
        int longEdgeMinLength = 30;

        bool filterLightAspectRatio = true;
        Range<double> lightAspectRatio = {2, 30};

        // ================================ Armor Filter ================================

        bool filterLightLengthRatio = true;
        double lightLengthMaxRatio = 1.5;

        bool filterLightXDistance = false;
        Range<double> lightXDistOverL = {1, 3};

        bool filterLightYDistance = false;
        Range<double> lightYDistOverL = {0, 1};

        bool filterLightAngleDiff = true;
        double lightAngleMaxDiff = 10;

        bool filterArmorAspectRatio = true;
        Range<double> armorAspectRatio = {1.25, 5};
    };

    ArmorDetector()
            : noteContours(cv::Scalar(0, 255, 255)) // yellow
    {}

    void setParams(const SharedParameters &shared, const ParameterSet &p) {
        sharedParams = shared;
        params = p;
    }

    const ParameterSet &getParams() const { return params; }

    vector<cv::Point2f> detect(const cv::Mat &img);

private:

    SharedParameters sharedParams;
    ParameterSet params;

    cv::Mat imgOriginal;
    cv::Mat imgGray;
    cv::Mat imgBrightnessThreshold;
    cv::Mat imgColorThreshold;
    cv::Mat imgLights;
#ifdef DEBUG
    AnnotatedMat noteContours;
#endif
    int acceptedContourCount;
    cv::Mat imgArmors;

    friend class MainWindow;

    static void drawRotatedRect(cv::Mat &img, const cv::RotatedRect &rect, const cv::Scalar &boarderColor);

    /**
     * Canonicalize a non-square rotated rect from cv::minAreaRect and make:
     *  width: the short edge
     *  height: the long edge
     *  angle: in [-90, 90). The angle is then the angle between the long edge and the vertical axis.
     *  points: consider the rect to be vertical. Bottom left is 0, and 1, 2, 3 clockwise
     *
     *            w               2       h       3
     *        1  ---  2             -------------
     *          |   |            w | angle = -90 |           (angle can't be 90)
     *          |   |  h            -------------
     *          |   |             1               0
     *           ---
     *        0      3
     *
     * @param rect
     */
    static void canonicalizeRotatedRect(cv::RotatedRect &rect);

};

}

#endif //META_VISION_SOLAIS_ARMORDETECTOR_H
