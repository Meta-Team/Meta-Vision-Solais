//
// Created by liuzikai on 2/21/21.
//

#ifndef META_VISION_SOLAIS_ANNOTATEDMAT_H
#define META_VISION_SOLAIS_ANNOTATEDMAT_H

#include "Common.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <utility>

namespace meta {

class AnnotatedMat {
public:

    explicit AnnotatedMat(cv::Scalar rectColor) : rectColor(std::move(rectColor)) {}

    /**
     * Set the base Mat by deep copying another Mat. This function clears all annotations.
     * @param m
     */
    void copyFrom(const cv::Mat &m);

    /**
     * Set the base Mat by converting color from another Mat. This function clears all annotations.
     * @param m
     */
    void convertFrom(const cv::Mat &m, cv::ColorConversionCodes code);

    void annotate(const cv::RotatedRect& rect, const string &comment);

    const cv::Mat &mat() const { return annotatedMat; }

    bool match(int x, int y, string& comment) const;

private:

    cv::Scalar rectColor;
    cv::Mat annotatedMat;

    struct Annotation {
        cv::RotatedRect rect;
        string comment;
    };

    vector<Annotation> annotations;

    void drawRotatedRect(const cv::RotatedRect& rect);

};

}

#endif //META_VISION_SOLAIS_ANNOTATEDMAT_H
