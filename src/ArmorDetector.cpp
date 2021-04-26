//
// Created by liuzikai on 2/6/21.
//

#include "ArmorDetector.h"

using namespace cv;

namespace meta {

vector<Point2f> ArmorDetector::detect(const Mat &img) {

    /*
     * Note: in this mega function, steps are wrapped with {} to reduce local variable pollution and make it easier to
     *       read. Please follow the convention.
     * Note: for debug purpose, allows some local variables and single-line compound statements.
     */

    // ================================ Setup ================================
    {
        assert(img.cols == params.image_width() && img.rows == params.image_height() && "Input image size unmatched");
        img.copyTo(imgOriginal);
    }

    // ================================ Brightness Threshold ================================
    {
        cvtColor(imgOriginal, imgGray, COLOR_BGR2GRAY);
        threshold(imgGray, imgBrightnessThreshold, params.brightness_threshold(), 255, THRESH_BINARY);
    }

    // ================================ Color Threshold ================================
    {
        if (params.color_threshold_mode() == ParamSet::HSV) {

            // Convert to HSV color space
            Mat hsvImg;
            cvtColor(imgOriginal, hsvImg, COLOR_BGR2HSV);

            if (params.enemy_color() == ParamSet::RED) {
                // Red color spreads over the 0 (180) boundary, so combine them
                Mat thresholdImg0, thresholdImg1;
                inRange(hsvImg, Scalar(0, 0, 0), Scalar(params.hsv_red_hue().max(), 255, 255), thresholdImg0);
                inRange(hsvImg, Scalar(params.hsv_red_hue().min(), 0, 0), Scalar(180, 255, 255), thresholdImg1);
                imgColorThreshold = thresholdImg0 | thresholdImg1;
            } else {
                inRange(hsvImg, Scalar(params.hsv_blue_bue().min(), 0, 0),
                        Scalar(params.hsv_blue_bue().max(), 255, 255),
                        imgColorThreshold);
            }

        } else {

            vector<Mat> channels;
            split(imgOriginal, channels);

            // Filter using channel subtraction
            int mainChannel = (params.enemy_color() == ParamSet::RED ? 2 : 0);
            int oppositeChannel = (params.enemy_color() == ParamSet::RED ? 0 : 2);
            subtract(channels[mainChannel], channels[oppositeChannel], imgColorThreshold);
            threshold(imgColorThreshold, imgColorThreshold, params.rb_channel_threshold(), 255, THRESH_BINARY);

        }

        // Color filter dilate
        if (params.color_dilate().enabled()) {
            Mat element = cv::getStructuringElement(
                    cv::MORPH_ELLIPSE,
                    cv::Size(params.color_dilate().val(), params.color_dilate().val()));
            dilate(imgColorThreshold, imgColorThreshold, element);
        }

        // Apply filter
        imgLights = imgBrightnessThreshold & imgColorThreshold;
    }

    // ================================ Find Contours ================================
    vector<RotatedRect> lightRects;
    {
#ifdef DEBUG
        // Make a colored copy since we need to draw rect onto it
        noteContours.convertFrom(imgLights, COLOR_GRAY2BGR);
#endif

        vector<vector<Point>> contours;
        findContours(imgLights, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        // Filter individual contours
        for (const auto &contour : contours) {

            // Filter pixel count
            if (params.contour_pixel_count().enabled()) {
                if (contour.size() < params.contour_pixel_count().val()) {
                    continue;
                }
            }

            // Filter area size
            if (params.contour_min_area().enabled()) {
                double area = contourArea(contour);
                if (area < params.contour_min_area().val()) {
                    continue;
                }
            }

            // Fit contour using a rotated rect
            RotatedRect rect;
            switch (params.contour_fit_function()) {
                case ParamSet::MIN_AREA_RECT:
                    rect = minAreaRect(contour);
                    break;
                case ParamSet::ELLIPSE:
                    // There should be at least 5 points to fit the ellipse
                    if (contour.size() < 5) continue;
                    rect = fitEllipse(contour);
                    break;
            }
            canonicalizeRotatedRect(rect);

            // Filter long edge min length
            double longEdgeLength = max(rect.size.width, rect.size.height);
            if (params.long_edge_min_length().enabled() && longEdgeLength < params.long_edge_min_length().val()) {
                continue;
            }

            // Filter aspect ratio
            double shortEdgeLength = min(rect.size.width, rect.size.height);
            if (params.light_aspect_ratio().enabled()) {
                double aspectRatio = longEdgeLength / shortEdgeLength;
                if (!inRange(aspectRatio, params.light_aspect_ratio())) {
                    continue;
                }
            }

            // Accept the rect
            lightRects.emplace_back(rect);
        }
    }
    acceptedContourCount = lightRects.size();

    // If there is less than two light contours, stop detection
    if (lightRects.size() < 2) {
        return {};
    }

    // Sort lights from left to right based on center X
    sort(lightRects.begin(), lightRects.end(),
         [](RotatedRect &a1, RotatedRect &a2) {
             return a1.center.x < a2.center.x;
         });

    /*
     * OpenCV coordinate: +x right, +y down
     */

    // ================================ Combine Lights to Armors ================================
    vector<Point2f> acceptedArmorCenters;
    {
        imgOriginal.copyTo(imgArmors);

        Point2f armorPoints[4];
        /*
         *              1 ----------- 2
         *            |*|             |*|
         * left light |*|             |*| right light
         *            |*|             |*|
         *              0 ----------- 3
         *
         * Edges (0, 1) and (2, 3) lie on inner edge
         */

        for (int leftLightIndex = 0; leftLightIndex < lightRects.size() - 1; ++leftLightIndex) {

            const RotatedRect &leftRect = lightRects[leftLightIndex];  // already canonicalized


            Point2f leftPoints[4];
            leftRect.points(leftPoints);
            // See canonicalizeRotatedRect for the order of leftPoints[]
            armorPoints[0] = leftPoints[3];
            armorPoints[1] = leftPoints[2];


            auto &leftCenter = leftRect.center;

            for (int rightLightIndex = leftLightIndex + 1; rightLightIndex < lightRects.size(); rightLightIndex++) {

                const RotatedRect &rightRect = lightRects[rightLightIndex];  // already canonicalized


                Point2f rightPoints[4];
                rightRect.points(rightPoints);
                // See canonicalizeRotatedRect for the order of rightPoints[]
                armorPoints[3] = rightPoints[0];
                armorPoints[2] = rightPoints[1];


                auto &rightCenter = rightRect.center;

                double leftLength = cv::norm(armorPoints[1] - armorPoints[0]);
                double rightLength = cv::norm(armorPoints[2] - armorPoints[3]);
                double averageLength = (leftLength + rightLength) / 2;

                // Filter long light length to short light length ratio
                if (params.filterLightLengthRatio) {
                    double lengthRatio = leftLength > rightLength ?
                                         leftLength / rightLength : rightLength / leftLength;  // >= 1
                    if (lengthRatio > params.lightLengthMaxRatio) continue;
                }

                // Filter central X's difference
                if (params.filterLightXDistance) {
                    double xDiffOverAvgL = abs(leftCenter.x - rightCenter.x) / averageLength;
                    if (!params.lightXDistOverL.contains(xDiffOverAvgL)) {
                        continue;
                    }
                }

                // Filter central Y's difference
                if (params.filterLightYDistance) {
                    double yDiffOverAvgL = abs(leftCenter.y - rightCenter.y) / averageLength;
                    if (!params.lightYDistOverL.contains(yDiffOverAvgL)) {
                        continue;
                    }
                }

                auto leftVector = armorPoints[1] - armorPoints[0];   // up
                assert(leftVector.y <= 0 && "leftVector should be upward");
                auto rightVector = armorPoints[2] - armorPoints[3];  // up
                assert(rightVector.y <= 0 && "rightVector should be upward");
                auto topVector = armorPoints[2] - armorPoints[1];    // right
                assert(topVector.x >= 0 && "topVector should be rightward");
                auto bottomVector = armorPoints[3] - armorPoints[0];  // right
                assert(bottomVector.x >= 0 && "bottomVector should be rightward");

                // Filter angle difference
                if (params.filterLightAngleDiff) {
                    double angleDiff = std::abs(leftRect.angle - rightRect.angle);
                    if (angleDiff > params.lightAngleMaxDiff) {
                        continue;
                    }
                }

                double armorHeight = (cv::norm(leftVector) + cv::norm(rightVector)) / 2;
                double armorWidth = (cv::norm(topVector) + cv::norm(bottomVector)) / 2;

                // Filter armor aspect ratio
                if (params.filterArmorAspectRatio) {
                    if (!params.armorAspectRatio.contains(armorWidth / armorHeight)) {
                        continue;
                    }
                }

                // Accept the armor
                Point2f center = {0, 0};
                for (int i = 0; i < 4; i++) {
                    center.x += armorPoints[i].x;
                    center.y += armorPoints[i].y;
                    // Draw in red
                    cv::line(imgArmors, armorPoints[i], armorPoints[(i + 1) % 4], Scalar(0, 0, 255), 2);
                }

                // Just use the average X and Y coordinate for the four point
                center.x /= 4;
                center.y /= 4;
                acceptedArmorCenters.emplace_back(center);
                cv::circle(imgArmors, center, 2, Scalar(0, 0, 255), 2);
            }
        }
    }
    return acceptedArmorCenters;
}

void ArmorDetector::drawRotatedRect(Mat &img, const RotatedRect &rect, const Scalar &boarderColor) {
    cv::Point2f vertices[4];
    rect.points(vertices);
    for (int i = 0; i < 4; i++) {
        cv::line(img, vertices[i], vertices[(i + 1) % 4], boarderColor, 2);
    }
}

void ArmorDetector::canonicalizeRotatedRect(cv::RotatedRect &rect) {
    // Reference: https://namkeenman.wordpress.com/2015/12/18/open-cv-determine-angle-of-rotatedrect-minarearect/
    // Except that angle may be in [0, 90)
    // The following code handle the most general cases

    // Nothing need to be done for rect.size.width < rect.size.height (case 1, 4, 5 in the URL above)

    if (rect.size.width > rect.size.height) {
        // Case 2, 3, 6
        std::swap(rect.size.width, rect.size.height);
        rect.angle += 90;
    }
    if (rect.angle >= 90) {
        // Include transforming case 7 (now become case 5 except that angle == 90) to case 5
        rect.angle -= 180;
    } else if (rect.angle < -90) {
        rect.angle += 180;
    }
    assert(rect.angle >= -90 && rect.angle < 90);
}

}