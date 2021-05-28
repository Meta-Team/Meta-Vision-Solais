//
// Created by liuzikai on 2/6/21.
//

#include "ArmorDetector.h"

using namespace cv;

namespace meta {

std::vector<Point2f> ArmorDetector::detect(const Mat &img) {

    /*
     * Note: in this mega function, steps are wrapped with {} to reduce local variable pollution and make it easier to
     *       read. Please follow the convention.
     * Note: for debug purpose, allows some local variables and single-line compound statements.
     */

    // ================================ Setup ================================
    {
        assert(img.cols == params.image_width() && img.rows == params.image_height() && "Input image size unmatched");
        img.copyTo(imgOriginal);  // do make a copy
        // TODO: this doesn't guarantee completion?
    }

    // ================================ Brightness Threshold ================================
    {
        cvtColor(imgOriginal, imgGray, COLOR_BGR2GRAY);
        threshold(imgGray, imgBrightness, params.brightness_threshold(), 255, THRESH_BINARY);
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
                imgColor = thresholdImg0 | thresholdImg1;
            } else {
                inRange(hsvImg, Scalar(params.hsv_blue_hue().min(), 0, 0),
                        Scalar(params.hsv_blue_hue().max(), 255, 255),
                        imgColor);
            }

        } else {

            std::vector<Mat> channels;
            split(imgOriginal, channels);

            // Filter using channel subtraction
            int mainChannel = (params.enemy_color() == ParamSet::RED ? 2 : 0);
            int oppositeChannel = (params.enemy_color() == ParamSet::RED ? 0 : 2);
            subtract(channels[mainChannel], channels[oppositeChannel], imgColor);
            threshold(imgColor, imgColor, params.rb_channel_threshold(), 255, THRESH_BINARY);

        }

        // Color filter dilate
        if (params.color_dilate().enabled()) {
            Mat element = cv::getStructuringElement(
                    cv::MORPH_ELLIPSE,
                    cv::Size(params.color_dilate().val(), params.color_dilate().val()));
            dilate(imgColor, imgColor, element);
        }

        // Apply filter
        imgLights = imgBrightness & imgColor;
    }

    // ================================ Find Contours ================================
    std::vector<RotatedRect> lightRects;
    {

        std::vector<std::vector<Point>> contours;
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
                default:
                    assert(!"Invalid params.contour_fit_function()");
            }
            canonicalizeRotatedRect(rect);
            // Now, width: the short edge, height: the long edge, angle: in [0, 180)

            // Filter long edge min length
            if (params.long_edge_min_length().enabled() && rect.size.height < params.long_edge_min_length().val()) {
                continue;
            }

            // Filter angle
            if (params.light_max_rotation().enabled() && std::min(rect.angle, 180 - rect.angle) >= params.light_max_rotation().val()) {
                continue;
            }

            // Filter aspect ratio
            if (params.light_aspect_ratio().enabled()) {
                double aspectRatio = rect.size.height / rect.size.width;
                if (!inRange(aspectRatio, params.light_aspect_ratio())) {
                    continue;
                }
            }

            // Accept the rect
            lightRects.emplace_back(rect);
        }
    }
    acceptedContourCount = lightRects.size();

    // FIXME: this may be costly in non-debug mode
    cv::cvtColor(imgLights, imgContours, COLOR_GRAY2BGR);
    for (const auto &rect : lightRects) {
        drawRotatedRect(imgContours, rect, Scalar(0, 255, 255));
    }

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
    std::vector<Point2f> acceptedArmorCenters;
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
            leftRect.points(leftPoints);  // bottomLeft, topLeft, topRight, bottomRight of unrotated rect
            if (leftRect.angle <= 90) {
                armorPoints[0] = leftPoints[3];
                armorPoints[1] = leftPoints[2];
            } else {
                armorPoints[0] = leftPoints[1];
                armorPoints[1] = leftPoints[0];
            }

            auto &leftCenter = leftRect.center;

            for (int rightLightIndex = leftLightIndex + 1; rightLightIndex < lightRects.size(); rightLightIndex++) {

                const RotatedRect &rightRect = lightRects[rightLightIndex];  // already canonicalized


                Point2f rightPoints[4];
                rightRect.points(rightPoints);  // bottomLeft, topLeft, topRight, bottomRight of unrotated rect
                if (rightRect.angle <= 90) {
                    armorPoints[3] = rightPoints[0];
                    armorPoints[2] = rightPoints[1];
                } else {  // case 1, 4, 5
                    armorPoints[3] = rightPoints[2];
                    armorPoints[2] = rightPoints[3];
                }


                auto leftVector = armorPoints[1] - armorPoints[0];   // up
                if (leftVector.y > 0) {
                    continue;  // leftVector should be upward, or lights intersect
                }
                auto rightVector = armorPoints[2] - armorPoints[3];  // up
                if (rightVector.y > 0) {
                    continue;  // rightVector should be upward, or lights intersect
                }
                auto topVector = armorPoints[2] - armorPoints[1];    // right
                if (topVector.x < 0) {
                    continue;  // topVector should be rightward, or lights intersect
                }
                auto bottomVector = armorPoints[3] - armorPoints[0];  // right
                if(bottomVector.x < 0) {
                    continue;  // bottomVector should be rightward, or lights intersect
                }


                auto &rightCenter = rightRect.center;

                double leftLength = cv::norm(armorPoints[1] - armorPoints[0]);
                double rightLength = cv::norm(armorPoints[2] - armorPoints[3]);
                double averageLength = (leftLength + rightLength) / 2;

                // Filter long light length to short light length ratio
                if (params.light_length_max_ratio().enabled()) {
                    double lengthRatio = leftLength > rightLength ?
                                         leftLength / rightLength : rightLength / leftLength;  // >= 1
                    if (lengthRatio > params.light_length_max_ratio().val()) continue;
                }

                // Filter central X's difference
                if (params.light_x_dist_over_l().enabled()) {
                    double xDiffOverAvgL = abs(leftCenter.x - rightCenter.x) / averageLength;
                    if (!inRange(xDiffOverAvgL, params.light_x_dist_over_l())) {
                        continue;
                    }
                }

                // Filter central Y's difference
                if (params.light_y_dist_over_l().enabled()) {
                    double yDiffOverAvgL = abs(leftCenter.y - rightCenter.y) / averageLength;
                    if (!inRange(yDiffOverAvgL, params.light_y_dist_over_l())) {
                        continue;
                    }
                }

                // Filter angle difference
                if (params.light_angle_max_diff().enabled()) {
                    double angleDiff = std::abs(leftRect.angle - rightRect.angle);
                    if (angleDiff > 90) {
                        angleDiff = 180 - angleDiff;
                    }
                    if (angleDiff > params.light_angle_max_diff().val()) {
                        continue;
                    }
                }

                double armorHeight = (cv::norm(leftVector) + cv::norm(rightVector)) / 2;
                double armorWidth = (cv::norm(topVector) + cv::norm(bottomVector)) / 2;

                // Filter armor aspect ratio
                if (params.armor_aspect_ratio().enabled()) {
                    if (!inRange(armorWidth / armorHeight, params.armor_aspect_ratio())) {
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

    // Assign (no copying) the result all at once, if the result is not being processed
    if (outputMutex.try_lock()) {
        imgOriginalOutput = imgOriginal;
        imgGrayOutput = imgGray;
        imgBrightnessOutput = imgBrightness;
        imgColorOutput = imgColor;
        imgLightsOutput = imgLights;
        imgContoursOutput = imgContours;
        imgArmorsOutput = imgArmors;
        outputMutex.unlock();
    }
    // Otherwise, simple discard the results of current run

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
    // https://stackoverflow.com/questions/15956124/minarearect-angles-unsure-about-the-angle-returned/21427814#21427814

    if (rect.size.width > rect.size.height) {
        std::swap(rect.size.width, rect.size.height);
        rect.angle += 90;
    }
    if (rect.angle < 0) {
        rect.angle += 180;
    } else if (rect.angle >= 180) {
        rect.angle -= 180;
    }
}

}