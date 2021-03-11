//
// Created by liuzikai on 3/6/21.
//

#ifndef META_VISION_SOLAIS_DETECTORTUNER_H
#define META_VISION_SOLAIS_DETECTORTUNER_H

#include "Common.h"
#include <filesystem>
#include <opencv2/highgui/highgui.hpp>

namespace meta {

namespace fs = std::filesystem;

class Camera;
class ArmorDetector;

class DetectorTuner {
public:

    /**
     * Construct a DetectorTuner.
     * @param detector
     * @param camera [optional]
     */
    explicit DetectorTuner(ArmorDetector *detector, Camera *camera) : detector(detector) {}

    int loadImageDataSet(const fs::path &path);

    const vector<fs::path> &getDataSetImages() const { return imageNames; }

    struct RunEvaluation {
        int imageCount = 0;
        int timeEscapedMS = 0;
    };

    void setSharedParams (const SharedParameters &shared) { sharedParams = shared; }

    void runOnSingleImage(const fs::path &imageName, vector<cv::Point2f> &armorCenters, RunEvaluation &evaluation);

    void runOnAllImages(RunEvaluation &evaluation);

    void runOnSingleCameraFrame(vector<cv::Point2f> &armorCenters, RunEvaluation &evaluation);

private:

    fs::path dataSetPath;
    vector<fs::path> imageNames;

    SharedParameters sharedParams;

    ArmorDetector *detector;
    Camera *camera;

};

}

#endif //META_VISION_SOLAIS_DETECTORTUNER_H
