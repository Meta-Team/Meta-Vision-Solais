//
// Created by liuzikai on 7/4/21.
//

#include "Camera.h"
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include "Utilities.h"

namespace meta {

bool Camera::startRecordToVideo(std::string &path, const package::ParamSet &params) {
    bool ret;
    videoWriterMutex.lock();
    {
        if (videoWriter.isOpened()) videoWriter.release();

        path += "/" + std::to_string(params.roi_width()) + "_" + std::to_string(params.roi_height()) + "_" +
                std::to_string(getFPS()) + "_" +
                (params.enemy_color() == package::ParamSet::BLUE ? "blue" : "red") + "_" + currentTimeString() +
                ".mkv";
#ifdef GSTREAMER_FOUND
        // https://stackoverflow.com/questions/43412797/opening-a-gstreamer-pipeline-from-opencv-with-videowriter
        // Use H264 instead for H265 for compatibility of videoWriter
        videoWriter.open(
                "appsrc ! autovideoconvert ! omxh264enc ! matroskamux ! filesink location=" + path + " sync=false",
                0, getFPS(), cv::Size(params.roi_width(), params.roi_height()), true);
#else
        videoWriter.open(path, cv::VideoWriter::fourcc('H', '2', '6', '4'), getFPS(),
                         cv::Size(params.roi_width(), params.roi_height()));
#endif
        ret = videoWriter.isOpened();
    }
    videoWriterMutex.unlock();
    recordingVideo = ret;
    return ret;
}

void Camera::stopRecordToVideo() {
    videoWriterMutex.lock();
    {
        videoWriter.release();
        recordingVideo = false;
    }
    videoWriterMutex.unlock();
}

}