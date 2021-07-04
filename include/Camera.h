//
// Created by liuzikai on 3/11/21.
//

#ifndef META_VISION_SOLAIS_CAMERA_H
#define META_VISION_SOLAIS_CAMERA_H

#include <thread>
#include "Parameters.pb.h"
#include "InputSource.h"
#include "CameraApi.h"
#include "Utilities.h"

namespace meta {

class Camera : public InputSource {
public:

    virtual bool open(const package::ParamSet &params) = 0;  // block until a valid frame is retrieved

    virtual std::string getCameraInfo() const = 0;

    virtual int getFPS() const = 0;

    virtual bool startRecordToVideo(std::string &path, const package::ParamSet &params);  // path will be changed to filename

    virtual void stopRecordToVideo();

    virtual bool isRecordingVideo() const { return recordingVideo; }

protected:

    cv::VideoWriter videoWriter;
    std::mutex videoWriterMutex;
    bool recordingVideo = false;  // for lock-free query isRecordingVideo()

};

class OpenCVCamera : public Camera {
public:

    ~OpenCVCamera() override;

    bool open(const package::ParamSet &params) override;

    bool isOpened() const override { return cap.isOpened(); }

    void close() override;

    std::string getCameraInfo() const override { return capInfoSS.str(); };

    int getFPS() const override { return (int) cap.get(cv::CAP_PROP_FPS); }

    TimePoint getFrameCaptureTime() const override { return bufferCaptureTime[lastBuffer]; }

    const cv::Mat &getFrame() const override { return buffer[lastBuffer]; }

private:

    cv::VideoCapture cap;

    std::stringstream capInfoSS;

    // Double buffering
    uint8_t lastBuffer = 0;
    cv::Mat buffer[2];
    TimePoint bufferCaptureTime[2] = {0, 0};

    std::thread *th = nullptr;
    bool threadShouldExit;

    void readFrameFromCamera(const package::ParamSet &params);
};

class MVCamera : public Camera {
public:

    ~MVCamera() override;

    bool open(const package::ParamSet &params) override;

    bool isOpened() const override { return (hCamera != 0); }

    void close() override;

    std::string getCameraInfo() const override { return capInfoSS.str(); };

    int getFPS() const override { return params.fps(); }

    TimePoint getFrameCaptureTime() const override { return bufferCaptureTime[lastBuffer]; }

    const cv::Mat &getFrame() const override { return buffer[lastBuffer]; }

    void fetchNextFrame() override;

private:

    int hCamera = 0;
    package::ParamSet params;

    std::stringstream capInfoSS;

    // Double buffering for processing
    bool shouldFetchNextFrame = true;
    uint8_t lastBuffer = 0;
    cv::Mat buffer[2];
    TimePoint bufferCaptureTime[2] = {0, 0};

    static void newFrameCallback(CameraHandle hCamera, BYTE *pFrameBuffer, tSdkFrameHead *pFrameHead, PVOID pContext);

};

}

#endif //META_VISION_SOLAIS_CAMERA_H
