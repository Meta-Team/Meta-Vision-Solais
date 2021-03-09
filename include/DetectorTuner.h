//
// Created by liuzikai on 3/6/21.
//

#ifndef META_VISION_SOLAIS_DETECTORTUNER_H
#define META_VISION_SOLAIS_DETECTORTUNER_H

#include "Common.h"
#include "ArmorDetector.h"
#include <filesystem>
#include <opencv2/highgui/highgui.hpp>

namespace meta {

namespace fs = std::filesystem;

class DetectorTuner {
public:

    explicit DetectorTuner(ArmorDetector *detector) : detector(detector) {}

    int loadImageDataSet(const fs::path &path);

    const vector<fs::path> &getImages() const { return imageNames; }

    void setDetectorParams(const ArmorDetector::ParameterSet &p) { detector->setParams(p); }

    struct RunEvaluation {
        int imageCount = 0;
        int timeEscapedMS = 0;
    };

    void runOnSingleImage(const fs::path &imageName, vector<cv::Point2f> &armorCenters, RunEvaluation &evaluation);

    void runOnAllImages(RunEvaluation &evaluation);

private:

    fs::path dataSetPath;
    vector<fs::path> imageNames;

    ArmorDetector *detector;

};

}

#endif //META_VISION_SOLAIS_DETECTORTUNER_H
