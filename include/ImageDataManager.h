//
// Created by liuzikai on 3/6/21.
//

#ifndef META_VISION_SOLAIS_DETECTORTUNER_H
#define META_VISION_SOLAIS_DETECTORTUNER_H

#include "Common.h"
#include <boost/filesystem.hpp>
#include <opencv2/highgui/highgui.hpp>

namespace meta {

namespace fs = boost::filesystem;

class ImageDataManager {
public:

    ImageDataManager();

    void reloadDataSetList();

    const vector<string> &getDataSetList() const { return dataSets; }

    int loadDataSet(const string &dataSetName);

    const vector<string> &getImageList() const { return images; }

    cv::Mat getImage(const string &imageName) const;

private:

    const fs::path imageSetRoot;

    fs::path currentDataSetPath;
    vector<string> dataSets;
    vector<string> images;
};

}

#endif //META_VISION_SOLAIS_DETECTORTUNER_H
