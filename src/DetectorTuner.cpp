//
// Created by liuzikai on 3/6/21.
//

#include "DetectorTuner.h"
#include "Camera.h"
#include "ArmorDetector.h"
#include <iostream>
#include <chrono>
#include <pugixml.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace meta {

int DetectorTuner::loadImageDataSet(const string &path) {
    dataSetPath = path;
    imageNames.clear();

    std::cout << "Loading data set " << path << "...\n";

    int imageCount = 0;
    for (const auto &entry : fs::directory_iterator(fs::path(path) / "JPEGImages")) {

        /* if (strcasecmp(entry.path().extension().c_str(), ".jpg") != 0) {
            std::cerr << "Non-image file: " << entry.path().filename() << "\n";
            continue;
        } */

        fs::path xmlFile = fs::path(path) / "Annotations" / entry.path().stem();
        xmlFile += ".xml";

        if (!fs::exists(xmlFile)) {
            std::cerr << "Missing xml file: " << entry.path().filename() << "\n";
            continue;
        }

        imageNames.emplace_back(entry.path().filename().string());

        imageCount++;
    }

    std::cout << imageCount << " images loaded.\n";

    return imageCount;
}

void DetectorTuner::runOnSingleImage(const string &imageName,
                                     vector<cv::Point2f> &armorCenters,
                                     DetectorTuner::RunEvaluation &evaluation) {

    fs::path imageFile = fs::path(dataSetPath) / "JPEGImages" / imageName;

    auto img = cv::imread(imageFile.string());

    if (img.cols != sharedParams.imageWidth || sharedParams.imageHeight) {
        cv::resize(img, img, cv::Size(sharedParams.imageWidth, sharedParams.imageHeight));
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    armorCenters = detector->detect(img);
    evaluation.timeEscapedMS = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
    evaluation.imageCount = 1;
}

}