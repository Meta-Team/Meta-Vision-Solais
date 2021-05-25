//
// Created by liuzikai on 3/11/21.
//

#ifndef META_VISION_SOLAIS_CAMERA_H
#define META_VISION_SOLAIS_CAMERA_H

#include <thread>
#include <atomic>
#include "Parameters.pb.h"
#include "VideoSource.h"

namespace meta {

class Camera : public VideoSource {
public:

    explicit Camera() : lastBuffer(0) {}

    ~Camera();

    bool open(const package::ParamSet &params) override;

    bool isOpened() const override { return cap.isOpened(); }

    std::string getCapInfo() const { return capInfoSS.str(); };

    void close() override;

    int getFrameID() const override { return bufferFrameID[lastBuffer]; }

    const cv::Mat &getFrame() const override { return buffer[lastBuffer]; }

    using NewFrameCallBack = void (*)(void *);

    void registerNewFrameCallBack(NewFrameCallBack callBack, void *param);


private:

    cv::VideoCapture cap;

    std::stringstream capInfoSS;

    // Double buffering
    std::atomic<int> lastBuffer;
    cv::Mat buffer[2];
    int bufferFrameID[2] = {0, 0};

    std::thread *th = nullptr;
    std::atomic<bool> threadShouldExit;

    std::vector<std::pair<NewFrameCallBack, void *>> callbacks;

    void readFrameFromCamera(const package::ParamSet &params);

};

}

#endif //META_VISION_SOLAIS_CAMERA_H
