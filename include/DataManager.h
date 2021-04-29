//
// Created by liuzikai on 3/6/21.
//

#ifndef META_VISION_SOLAIS_DETECTORTUNER_H
#define META_VISION_SOLAIS_DETECTORTUNER_H

#include "Common.h"
#include "Parameters.h"
#include <boost/filesystem.hpp>
#include <opencv2/highgui/highgui.hpp>

namespace meta {

namespace fs = boost::filesystem;

class DataManager {
public:

    DataManager();

    void reloadDataSetList();

    const vector<string> &getDataSetList() const { return dataSets; }

    int loadDataSet(const string &dataSetName);

    const vector<string> &getImageList() const { return images; }

    cv::Mat getImage(const string &imageName) const;

    void reloadParamSetList();  // switch to default

    const vector<string> &getParamSetList() const { return paramsSetNames; }

    void switchToParamSet(const string &paramSetName) { curParamSetName = paramSetName; }

    const string &currentParamSetName() const { return curParamSetName; }

    ParamSet loadCurrentParamSet() const;

    void saveCurrentParamSet(const ParamSet &p);

private:

    const fs::path imageSetRoot;

    fs::path currentDataSetPath;
    vector<string> dataSets;      // directory name
    vector<string> images;        // jpg filenames

    const fs::path paramSetRoot;
    vector<string> paramsSetNames;    // json filenames without the extension
    string curParamSetName;           // json filenames without the extension

    void saveParamSetToJson(const ParamSet &p, const fs::path &filename);
};

}

#endif //META_VISION_SOLAIS_DETECTORTUNER_H
