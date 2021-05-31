//
// Created by liuzikai on 5/25/21.
//

#include "ImageSet.h"
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>

namespace meta {

// DATA_SET_ROOT defined in CMakeLists.txt
ImageSet::ImageSet() : imageSetRoot(fs::path(DATA_SET_ROOT) / "images") {
    if (!fs::exists(imageSetRoot)) {
        fs::create_directories(imageSetRoot);
    }
}

void ImageSet::reloadImageSetList() {
    imageSets.clear();
    currentImageSetPath = "";
    images.clear();
    if (fs::is_directory(imageSetRoot)) {
        for (const auto &entry : fs::directory_iterator(imageSetRoot)) {
            if (fs::is_directory(imageSetRoot / entry.path().filename())) {
                imageSets.emplace_back(entry.path().filename().string());
            }
        }
    }
}

int ImageSet::switchImageSet(const std::string &dataSetName) {
    if (isOpened()) close();

    currentImageSetPath = imageSetRoot / dataSetName;
    images.clear();

    std::cout << "ImageSet: loading data set " << currentImageSetPath << "..." << std::endl;

    for (const auto &entry : fs::directory_iterator(currentImageSetPath)) {
        if (strcasecmp(entry.path().extension().c_str(), ".jpg") == 0) {

            fs::path xmlFile = fs::path(currentImageSetPath) / entry.path().stem();
            xmlFile += ".xml";

            if (!fs::exists(xmlFile)) {
                std::cerr << "Missing xml file: " << entry.path().filename() << std::endl;
                // continue;
            }

            images.emplace_back(entry.path().filename().string());
        }
    }

    std::sort(images.begin(), images.end());

    std::cout << "ImageSet: " << images.size() << " images loaded." << std::endl;

    return images.size();
}

cv::Mat ImageSet::getSingleImage(const std::string &imageName, const ParamSet &params) const {
    if (currentImageSetPath.empty()) return cv::Mat();
    fs::path imageFile = fs::path(currentImageSetPath) / imageName;
    auto img = cv::imread(imageFile.string());
    if (img.rows != params.image_height() || img.cols != params.image_width()) {
        cv::resize(img, img, cv::Size(params.image_width(), params.image_height()));
    }
    return img;
}

bool ImageSet::open(const ParamSet &params) {
    if (th) close();

    if (currentImageSetPath.empty()) {
        std::cerr << "ImageSet: failed to open as no image set is selected\n";
        return false;
    }

    std::cout << "ImageSet: loading " << images.size() << " images into memory\n";
    imageMats.clear();
    imageMats.reserve(images.size());
    for (const auto &image : images) {
        fs::path imageFile = fs::path(currentImageSetPath) / image;
        auto img = cv::imread(imageFile.string());
        if (img.rows != params.image_height() || img.cols != params.image_width()) {
            cv::resize(img, img, cv::Size(params.image_width(), params.image_height()));
        }
        imageMats.emplace_back(img);
    }

    threadShouldExit = false;
    th = new std::thread(&ImageSet::loadFrameFromImageSet, this, params);
    return true;
}

void ImageSet::loadFrameFromImageSet(const ParamSet &params) {
    shouldFetchNextFrame = true;

    auto it = imageMats.begin();  // next frame iterator
    while (true) {

        while (!shouldFetchNextFrame && !threadShouldExit) std::this_thread::yield();

        uint8_t workingBuffer = 1 - lastBuffer;

        if (threadShouldExit || it == imageMats.end()) {  // no more image
            bufferFrameID[workingBuffer] = -1;  // indicate invalid frame
            break;
        }

        // Set the image
        buffer[workingBuffer] = *it;
        ++it;

        // Increment frame ID
        bufferFrameID[workingBuffer] = bufferFrameID[lastBuffer] + 1;
        if (bufferFrameID[workingBuffer] > FRAME_ID_MAX) bufferFrameID[workingBuffer] = 0;

        // Switch
        lastBuffer = workingBuffer;

        // The only place of incrementing
        ++cumulativeFrameCounter;

        shouldFetchNextFrame = false;
    }

    imageMats.clear();
    std::cout << "ImageSet: closed\n";
}


void ImageSet::fetchNextFrame() {
    while (shouldFetchNextFrame) {  // check for flag set last time
        std::this_thread::yield();
    }
    shouldFetchNextFrame = true;
}

void ImageSet::close() {
    if (th) {
        threadShouldExit = true;
        th->join();
        delete th;
        th = nullptr;
    }
}

std::string ImageSet::currentTimeString() {
    // Reference: https://stackoverflow.com/questions/24686846/get-current-time-in-milliseconds-or-hhmmssmmm-format

    using namespace std::chrono;

    // Get current time
    auto now = system_clock::now();

    // Get number of milliseconds for the current second
    // (remainder after division into seconds)
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    // Convert to std::time_t in order to convert to std::tm (broken time)
    auto timer = system_clock::to_time_t(now);

    // Convert to broken time
    std::tm bt = *std::localtime(&timer);

    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y_%m_%d_%H_%M_%S");

    return oss.str();
}

std::string ImageSet::saveCapturedImage(const cv::Mat &image, const package::ParamSet &params) {
    // Directory: <width>_<height>_blue/red
    fs::path dir = imageSetRoot /
            fs::path(std::to_string(params.image_width()) + "_" + std::to_string(params.image_height()) + "_" +
            (params.enemy_color() == package::ParamSet_EnemyColor_BLUE ? "blue" : "red"));

    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }

    // Filename: formatted current time with milliseconds
    fs::path basename = dir / fs::path(currentTimeString());
    fs::path filename = basename;
    filename += ".jpg";
    if (fs::exists(filename)) {
        int i = -1;
        do {
            i++;
            filename = basename;
            filename += "_" + std::to_string(i) + ".jpg";
        } while (fs::exists(filename));
    }

    // Save image
    cv::imwrite(filename.string(), image);

    // Let the user reload the image list

    return filename.string();
}

}