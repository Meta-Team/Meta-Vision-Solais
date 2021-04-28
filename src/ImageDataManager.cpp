//
// Created by liuzikai on 3/6/21.
//

#include "ImageDataManager.h"
#include <iostream>
#include <pugixml.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace meta {

int ImageDataManager::loadDataSet(const string &path) {
    dataSetPath = path;
    imageNames.clear();

    std::cout << "ImageDataManager: Loading data set " << path << "..." << std::endl;

    int imageCount = 0;
    for (const auto &entry : fs::directory_iterator(fs::path(path) / "JPEGImages")) {

        /* if (strcasecmp(entry.path().extension().c_str(), ".jpg") != 0) {
            std::cerr << "Non-image file: " << entry.path().filename() << "\n";
            continue;
        } */

        fs::path xmlFile = fs::path(path) / "Annotations" / entry.path().stem();
        xmlFile += ".xml";

        if (!fs::exists(xmlFile)) {
            std::cerr << "Missing xml file: " << entry.path().filename() << std::endl;
            continue;
        }

        imageNames.emplace_back(entry.path().filename().string());

        imageCount++;
    }

    std::cout << "ImageDataManager: " << imageCount << " images loaded." << std::endl;

    return imageCount;
}

cv::Mat ImageDataManager::getImage(const string &imageName) const {
    fs::path imageFile = fs::path(dataSetPath) / "JPEGImages" / imageName;
    return cv::imread(imageFile.string());
}

}