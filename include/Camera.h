//
// Created by liuzikai on 3/11/21.
//

#ifndef META_VISION_SOLAIS_CAMERA_H
#define META_VISION_SOLAIS_CAMERA_H

#include <thread>
#include "Parameters.pb.h"
#include "VideoSource.h"

namespace meta {

/**
 * @name CameraCore
 * @brief Abstract interface for camera backend, defined like OpenCV VideoCapture API.
 */
class CameraCore {
public:

    virtual ~CameraCore() {}

    virtual bool open(int index, int apiPreference = cv::CAP_ANY) = 0;

    virtual bool isOpened() const = 0;

    virtual bool set(int propId, double value) = 0;

    virtual double get(int propId) const = 0;

    virtual bool read(cv::Mat &image) = 0;

    virtual void release() = 0;

};

/**
 * @name CameraCoreOpenCV
 * @brief Camera backend of OpenCV VideoCapture.
 */
class CameraCoreOpenCV : public CameraCore {
public:

    bool open(int index, int apiPreference = cv::CAP_ANY) override { return cap.open(index, apiPreference); }

    bool isOpened() const override { return cap.isOpened(); }

    bool set(int propId, double value) override { return cap.set(propId, value); }

    double get(int propId) const override { return cap.get(propId); }

    bool read(cv::Mat &image) override { return cap.read(image); }

    void release() override { cap.release(); }

private:
    cv::VideoCapture cap;
};

/**
 * @name CameraCoreMVCamera
 * @brief Camera backend of MindVision camera.
 */
class CameraCoreMVCamera : public CameraCore {
public:
    ~CameraCoreMVCamera();

    bool open(int index, int apiPreference = cv::CAP_ANY) override;

    bool isOpened() const override;

    bool set(int propId, double value) override;

    double get(int propId) const override;

    bool read(cv::Mat &image) override;

    void release() override;

protected:

    int hCamera = 0;
    unsigned char *matBuffer = nullptr;  // buffer for cv::Mat

};

class Camera : public VideoSource {
public:

    ~Camera() override;

    // Block until a valid frame is retrieved
    bool open(const package::ParamSet &params) override;

    bool isOpened() const override { return cap != nullptr && cap->isOpened(); }

    std::string getCapInfo() const { return capInfoSS.str(); };

    void close() override;

    TimePoint getFrameCaptureTime() const override { return bufferCaptureTime[lastBuffer]; }

    const cv::Mat &getFrame() const override { return buffer[lastBuffer]; }

private:

    CameraCore *cap = nullptr;

    std::stringstream capInfoSS;

    // Double buffering
    uint8_t lastBuffer = 0;
    cv::Mat buffer[2];
    TimePoint bufferCaptureTime[2] = {TimePoint(), TimePoint()};

    std::thread *th = nullptr;
    bool threadShouldExit;

    void readFrameFromCamera(const package::ParamSet &params);

};

}

#endif //META_VISION_SOLAIS_CAMERA_H
