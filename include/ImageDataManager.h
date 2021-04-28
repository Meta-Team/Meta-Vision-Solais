//
// Created by liuzikai on 3/6/21.
//

#ifndef META_VISION_SOLAIS_DETECTORTUNER_H
#define META_VISION_SOLAIS_DETECTORTUNER_H

#include "Common.h"
#include <opencv2/highgui/highgui.hpp>

namespace meta {

class ImageDataManager {
public:

    int loadDataSet(const string &path);

    const vector<string> &getImageNames() const { return imageNames; }

    cv::Mat getImage(const string &imageName) const;

private:

    string dataSetPath;
    vector<string> imageNames;
};

}

#endif //META_VISION_SOLAIS_DETECTORTUNER_H
