//
// Created by liuzikai on 3/6/21.
//

#ifndef META_VISION_SOLAIS_DETECTORTUNER_H
#define META_VISION_SOLAIS_DETECTORTUNER_H

#include "Common.h"
#include <opencv2/highgui/highgui.hpp>

namespace meta {

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

    int loadImageDataSet(const string &path);

    const vector<string> &getDataSetImages() const { return imageNames; }

    struct RunEvaluation {
        int imageCount = 0;
        int timeEscapedMS = 0;
    };

    void setSharedParams (const SharedParameters &shared) { sharedParams = shared; }

    void runOnSingleImage(const string &imageName, vector<cv::Point2f> &armorCenters, RunEvaluation &evaluation);

    void runOnAllImages(RunEvaluation &evaluation);

    void runOnSingleCameraFrame(vector<cv::Point2f> &armorCenters, RunEvaluation &evaluation);

private:

    string dataSetPath;
    vector<string> imageNames;

    SharedParameters sharedParams;

    ArmorDetector *detector;
    Camera *camera;

};

}

#endif //META_VISION_SOLAIS_DETECTORTUNER_H
