//
// Created by liuzikai on 2/6/21.
//

#include "ArmorDetector.h"
#include <vector>

using namespace cv;
using std::vector;

namespace meta {

void ArmorDetector::detect(const cv::Mat &img) {
    imgOriginal = img;

    // ================================ Brightness Threshold ================================
    cvtColor(imgOriginal, imgGray, COLOR_BGR2GRAY);
    threshold(imgGray, imgBrightnessThreshold, params.brightnessThreshold, 255, THRESH_BINARY);

    // ================================ Color Threshold ================================
    {
        if (params.colorThresholdMode == HSV) {

            // Convert to HSV color space
            Mat hsvImg;
            cvtColor(imgOriginal, hsvImg, COLOR_BGR2HSV);

            // TODO: parameterized S and V?
            if (params.targetColor == RED) {
                // Red color spreads over the 0 (180) boundary, so combine them
                Mat thresholdImg0, thresholdImg1;
                inRange(hsvImg, Scalar(0, 0, 0), Scalar(params.hsvRedHue.max, 255, 255), thresholdImg0);
                inRange(hsvImg, Scalar(params.hsvRedHue.min, 0, 0), Scalar(180, 255, 255), thresholdImg1);
                imgColorThreshold = thresholdImg0 | thresholdImg1;
            } else {
                inRange(hsvImg, Scalar(params.hsvBlueHue.min, 0, 0), Scalar(params.hsvBlueHue.max, 255, 255),
                        imgColorThreshold);
            }

        } else {

            vector<Mat> channels;
            split(imgOriginal, channels);

            // Filter using channel subtraction
            int mainChannel = (params.targetColor == RED ? 2 : 0);
            int oppositeChannel = (params.targetColor == RED ? 0 : 2);
            subtract(channels[mainChannel], channels[oppositeChannel], imgColorThreshold);
            threshold(imgColorThreshold, imgColorThreshold, params.rbChannelThreshold, 255, THRESH_BINARY);

        }

        // Color filter dilate
        if (params.colorDilate > 0) {
            Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(params.colorDilate, params.colorDilate));
            dilate(imgColorThreshold, imgColorThreshold, element);
        }

        // Apply filter
        imgLights = imgBrightnessThreshold & imgColorThreshold;
    }

    // ================================ Find Contours ================================
    vector<RotatedRect> lightRects;
    {
        // Make a colored copy since we need to draw rect onto it
        cvtColor(imgLights, imgContours, COLOR_GRAY2BGR);

        vector<vector<Point>> contours;
        findContours(imgLights, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        // Filter individual contours
        for (const auto &contour : contours) {

            // Filter rect size
            if (params.contourRectMinSize != 0 && contour.size() < params.contourRectMinSize) continue;

            // Filter area size
            if (params.contourMinArea != 0 || contourArea(contour) < params.contourMinArea) continue;

            // Fit contour using a rotated rect
            RotatedRect rect = minAreaRect(contour);

            // Filter long edge min length
            double longEdgeLength = max(rect.size.width, rect.size.height);
            if (params.longEdgeMinLength != 0 && longEdgeLength < params.longEdgeMinLength) {
                continue;
            }

            // Filter aspect ratio
            double shortEdgeLength = min(rect.size.width, rect.size.height);
            if (params.enableAspectRatioFilter) {
                double aspectRatio = longEdgeLength / shortEdgeLength;
                if (!params.aspectRatio.inRange(aspectRatio)) {
                    continue;
                }
            }

            // Accept the rect
            lightRects.emplace_back(rect);
            drawRotatedRect(imgContours, rect, Scalar(0, 255, 255));  // yellow
        }
    }

}

void ArmorDetector::drawRotatedRect(Mat &img, const RotatedRect &rect, const Scalar &boarderColor) {
    cv::Point2f vertices[4];
    rect.points(vertices);
    for (int i = 0; i < 4; i++) {
        cv::line(img, vertices[i], vertices[(i + 1) % 4], boarderColor, 2);
    }
}

}