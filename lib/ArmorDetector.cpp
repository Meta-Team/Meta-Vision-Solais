//
// Created by liuzikai on 2/6/21.
//

#include "ArmorDetector.h"
#include <vector>
using namespace cv;
using std::vector;

namespace meta {

void ArmorDetector::detect(const cv::Mat &img) {
    // Set original image
    imgOriginal = img;

    // Convert to gray
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
                inRange(hsvImg, Scalar(0, 0, 0), Scalar(params.hsvRedMax, 255, 255), thresholdImg0);
                inRange(hsvImg, Scalar(params.hsvRedMin, 0, 0), Scalar(180, 255, 255), thresholdImg1);
                imgColorThreshold = thresholdImg0 | thresholdImg1;
            } else {
                inRange(hsvImg, Scalar(params.hsvBlueMin, 0, 0), Scalar(params.hsvBlueMax, 255, 255), imgColorThreshold);
            }

        } else {

            vector<Mat> channels;
            split(imgOriginal, channels);

            int mainChannel = (params.targetColor == RED ? 2 : 0);
            int oppositeChannel = (params.targetColor == RED ? 0 : 2);
            subtract(channels[mainChannel], channels[oppositeChannel], imgColorThreshold);
            threshold(imgColorThreshold, imgColorThreshold, params.rbChannelThreshold, 255, THRESH_BINARY);

            Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
            dilate(imgColorThreshold, imgColorThreshold, element);
        }

        imgLights = imgBrightnessThreshold & imgColorThreshold;
    }

    // ================================ Find Contours ================================
    vector<RotatedRect> lightRects;
    {
        // Make a colored copy since we need to draw rect onto it
        cvtColor(imgLights, imgContours, COLOR_GRAY2BGR);

        vector<vector<Point>> contours;
        findContours(imgLights, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        for(const auto& contour : contours) {

            // Eliminate contour that is too small
            if (contour.size() < 5 || contourArea(contour) < 10) continue;

            // fit the lamp contour as a eclipse
            RotatedRect rrect = minAreaRect(contour);
            drawRotatedRect(imgContours, rrect, Scalar(0, 255, 255));  // 黄色
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