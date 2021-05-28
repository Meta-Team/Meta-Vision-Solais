//
// Created by liuzikai on 5/28/21.
//

#ifndef META_VISION_SOLAIS_GSTREAMERCAMERA_H
#define META_VISION_SOLAIS_GSTREAMERCAMERA_H

#include <thread>
#include <gst/gst.h>
#include <gst/gstinfo.h>
#include <gst/app/gstappsink.h>
#include <glib-unix.h>
#include "Parameters.pb.h"
#include "VideoSource.h"

namespace meta {

class GStreamerCamera : public VideoSource {
public:

    GStreamerCamera();

    ~GStreamerCamera() override;

    bool open(const package::ParamSet &params) override;

    bool isOpened() const override { return cap.isOpened(); }

    std::string getCapInfo() const { return capInfoSS.str(); };

    void close() override;

    int getFrameID() const override { return bufferFrameID[lastBuffer]; }

    const cv::Mat &getFrame() const override { return buffer[lastBuffer]; }

private:

    cv::VideoCapture cap;

    std::stringstream capInfoSS;

    // Double buffering
    uint8_t lastBuffer = 0;
    cv::Mat buffer[2];
    int bufferFrameID[2] = {0, 0};

    std::thread *th = nullptr;
    bool threadShouldExit;

    void readFrameFromCamera(const package::ParamSet &params);

    void handleAppSinkEOS(GstAppSink *appSink, gpointer userData);

    GstFlowReturn handleNewBuffer(GstAppSink *appSink, gpointer userData);

};

}

#endif //META_VISION_SOLAIS_GSTREAMERCAMERA_H
