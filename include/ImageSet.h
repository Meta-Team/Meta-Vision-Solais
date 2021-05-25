//
// Created by liuzikai on 5/25/21.
//

#ifndef META_VISION_SOLAIS_IMAGESET_H
#define META_VISION_SOLAIS_IMAGESET_H

#include <thread>
#include <boost/filesystem.hpp>
#include "Parameters.h"
#include "VideoSource.h"

namespace meta {

namespace fs = boost::filesystem;

class ImageSet : public VideoSource {
public:

    ImageSet();

    void reloadImageSetList();

    int switchImageSet(const std::string &dataSetName);

    const std::vector<std::string> &getImageSetList() const { return imageSets; }

    const std::vector<std::string> &getImageList() const { return images; }

    cv::Mat getSingleImage(const std::string &imageName, const ParamSet &params) const;

    bool open(const package::ParamSet &params) override;

    bool isOpened() const override { return (th != nullptr); }

    void close() override;

    int getFrameID() const override { return bufferFrameID[lastBuffer]; }

    const cv::Mat &getFrame() const override { return buffer[lastBuffer]; }

    void fetchNextFrame() override;

protected:

    const fs::path imageSetRoot;

    fs::path currentImageSetPath;
    std::vector<std::string> imageSets;      // directory name
    std::vector<std::string> images;         // jpg filenames

    // Double buffering
    std::atomic<bool> shouldFetchNextFrame;
    std::atomic<int> lastBuffer;
    cv::Mat buffer[2];
    int bufferFrameID[2] = {0, 0};

    std::thread *th = nullptr;
    std::atomic<bool> threadShouldExit;

    void loadFrameFromImageSet(const ParamSet &params);
};

}

#endif //META_VISION_SOLAIS_IMAGESET_H
