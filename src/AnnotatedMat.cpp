//
// Created by liuzikai on 2/21/21.
//

#include "AnnotatedMat.h"
#include <array>

namespace meta {

void AnnotatedMat::annotate(const cv::RotatedRect &rect, const string &comment) {
    drawRotatedRect(rect);
    annotations.push_back({rect, comment});
}

bool AnnotatedMat::match(int x, int y, string &comment) const {
    // Binary search may be faster, but ideally the number of annotations should not be large, so anyway...
    for (const auto &annotation : annotations) {
        std::array<cv::Point2f, 4> points;
        annotation.rect.points(points.data());
        if (cv::pointPolygonTest(points, cv::Point2f(x, y), false) >= 0) {
            // +1 for inside, 0 for on the edge
            comment = annotation.comment;
            return true;
        }
    }
    return false;
}

void AnnotatedMat::drawRotatedRect(const cv::RotatedRect &rect) {
    cv::Point2f vertices[4];
    rect.points(vertices);
    for (int i = 0; i < 4; i++) {
        cv::line(annotatedMat, vertices[i], vertices[(i + 1) % 4], rectColor, 2);
    }
}

void AnnotatedMat::copyFrom(const cv::Mat &m) {
    m.copyTo(annotatedMat);
    annotations.clear();
}

void AnnotatedMat::convertFrom(const cv::Mat &m, cv::ColorConversionCodes code) {
    cv::cvtColor(m, annotatedMat, code);
    annotations.clear();
}

}