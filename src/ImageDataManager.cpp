//
// Created by liuzikai on 3/6/21.
//

#include "ImageDataManager.h"
#include <iostream>
#include <pugixml.hpp>

namespace meta {

ImageDataManager::ImageDataManager()
        : imageSetRoot(fs::path(DATA_SET_ROOT) / "images") {
    reloadDataSetList();
}

void ImageDataManager::reloadDataSetList() {
    dataSets.clear();
    images.clear();
    if (fs::is_directory(imageSetRoot)) {
        for (const auto &entry : fs::directory_iterator(imageSetRoot)) {
            if (fs::is_directory(imageSetRoot / entry.path().filename())) {
                dataSets.emplace_back(entry.path().filename().string());
            }
        }
    }
}

int ImageDataManager::loadDataSet(const string &dataSetName) {
    currentDataSetPath = imageSetRoot / dataSetName;
    images.clear();

    std::cout << "ImageDataManager: Loading data set " << currentDataSetPath << "..." << std::endl;

    for (const auto &entry : fs::directory_iterator(currentDataSetPath)) {
        if (strcasecmp(entry.path().extension().c_str(), ".jpg") == 0) {

            fs::path xmlFile = fs::path(currentDataSetPath) / entry.path().stem();
            xmlFile += ".xml";

            if (!fs::exists(xmlFile)) {
                std::cerr << "Missing xml file: " << entry.path().filename() << std::endl;
                continue;
            }

            images.emplace_back(entry.path().filename().string());
        }
    }

    std::cout << "ImageDataManager: " << images.size() << " images loaded." << std::endl;

    return images.size();
}

cv::Mat ImageDataManager::getImage(const string &imageName) const {
    fs::path imageFile = fs::path(currentDataSetPath) / imageName;
    return cv::imread(imageFile.string());
}

}