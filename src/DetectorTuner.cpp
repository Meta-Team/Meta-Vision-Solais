//
// Created by liuzikai on 3/6/21.
//

#include "DetectorTuner.h"
#include "Camera.h"
#include "ArmorDetector.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <pugixml.hpp>

namespace fs = std::filesystem;

namespace meta {

int DetectorTuner::loadImageDataSet(const std::filesystem::path &path) {
    dataSetPath = path;
    imageNames.clear();

    std::cout << "Loading data set " << path << "...\n";

    int imageCount = 0;
    for (const auto &entry : std::filesystem::directory_iterator(path / "JPEGImages")) {

        /* if (strcasecmp(entry.path().extension().c_str(), ".jpg") != 0) {
            std::cerr << "Non-image file: " << entry.path().filename() << "\n";
            continue;
        } */

        fs::path xmlFile = path / "Annotations" / entry.path().stem();
        xmlFile += ".xml";

        if (!fs::exists(xmlFile)) {
            std::cerr << "Missing xml file: " << entry.path().filename() << "\n";
            continue;
        }

        imageNames.emplace_back(entry.path().filename());

        imageCount++;
    }

    std::cout << imageCount << " images loaded.\n";

    return imageCount;
}

void DetectorTuner::runOnSingleImage(const fs::path &imageName,
                                     vector<cv::Point2f> &armorCenters,
                                     DetectorTuner::RunEvaluation &evaluation) {

    fs::path imageFile = dataSetPath / "JPEGImages" / imageName;

    auto img = cv::imread(imageFile);

    if (img.cols != sharedParams.imageWidth || sharedParams.imageHeight) {
        cv::resize(img, img, cv::Size(sharedParams.imageWidth, sharedParams.imageHeight));
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    armorCenters = detector->detect(img);
    evaluation.timeEscapedMS = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
    evaluation.imageCount = 1;
}

}