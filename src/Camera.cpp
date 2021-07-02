//
// Created by liuzikai on 3/11/21.
//

#include "Camera.h"
#include "CameraApi.h"
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>

namespace meta {

bool CameraCoreMVCamera::open(int index, int apiPreference) {
    CameraSdkStatus res;
    int cameraCount = 1;
    tSdkCameraDevInfo cameraEnumList;
    tSdkCameraCapbility capability;

    CameraSdkInit(1);

    res = CameraEnumerateDevice(&cameraEnumList, &cameraCount);
    if (res != CAMERA_STATUS_SUCCESS) {
        std::cerr << "CameraCoreMVCamera: CameraEnumerateDevice returned " << res << std::endl;
        return false;
    }

    if (cameraCount == 0) {
        std::cerr << "CameraCoreMVCamera: no MindVision camera found" << std::endl;
        return false;
    }

    res = CameraInit(&cameraEnumList, -1, -1, &hCamera);
    if (res != CAMERA_STATUS_SUCCESS) {
        std::cerr << "CameraCoreMVCamera: CameraInit returned " << res << std::endl;
        return false;
    }

    res = CameraGetCapability(hCamera, &capability);
    if (res != CAMERA_STATUS_SUCCESS) {
        std::cerr << "CameraCoreMVCamera: CameraGetCapability returned " << res << std::endl;
        return false;
    }

    matBuffer = (unsigned char *) malloc(
            capability.sResolutionRange.iHeightMax * capability.sResolutionRange.iWidthMax * 3);
    if (matBuffer == nullptr) {
        std::cerr << "CameraCoreMVCamera: failed to malloc matBuffer" << std::endl;
        return false;
    }

    res = CameraPlay(hCamera);
    if (res != CAMERA_STATUS_SUCCESS) {
        std::cerr << "CameraCoreMVCamera: CameraPlay returned " << res << std::endl;
        return false;
    }

    res = CameraSetOnceWB(hCamera);
    if (res != CAMERA_STATUS_SUCCESS) {
        std::cerr << "CameraCoreMVCamera: CameraSetOnceWB returned " << res << std::endl;
        return false;
    }

    res = CameraSetIspOutFormat(hCamera, CAMERA_MEDIA_TYPE_BGR8);
    if (res != CAMERA_STATUS_SUCCESS) {
        std::cerr << "CameraCoreMVCamera: CameraSetIspOutFormat returned " << res << std::endl;
        return false;
    }

    return true;
}

bool CameraCoreMVCamera::isOpened() const {
    return matBuffer != nullptr;
}

bool CameraCoreMVCamera::read(cv::Mat &image) {
    CameraSdkStatus res;
    tSdkFrameHead frameInfo;
    BYTE *rawBuffer = nullptr;

    res = CameraGetImageBuffer(hCamera, &frameInfo, &rawBuffer, 1000);
    if (res != CAMERA_STATUS_SUCCESS) {
        std::cerr << "CameraCoreMVCamera: CameraGetImageBuffer returned " << res << std::endl;
        return false;
    }

    if (image.cols != frameInfo.iWidth || image.rows != frameInfo.iHeight) {
        image = cv::Mat(cv::Size(frameInfo.iWidth, frameInfo.iHeight), CV_8UC3);
    }

    res = CameraImageProcess(hCamera, rawBuffer, image.data, &frameInfo);  // load directly into buffer
    if (res != CAMERA_STATUS_SUCCESS) {
        std::cerr << "CameraCoreMVCamera: CameraImageProcess returned " << res << std::endl;
        return false;
    }

    res = CameraReleaseImageBuffer(hCamera, rawBuffer);
    if (res != CAMERA_STATUS_SUCCESS) {
        std::cerr << "CameraCoreMVCamera: CameraReleaseImageBuffer returned " << res << std::endl;
        return false;
    }

    return true;
}

void CameraCoreMVCamera::release() {
    CameraUnInit(hCamera);
    hCamera = 0;
    free(matBuffer);  // free after CameraUnInit
    matBuffer = nullptr;
}

CameraCoreMVCamera::~CameraCoreMVCamera() {
    if (hCamera) CameraUnInit(hCamera);
    if (matBuffer) free(matBuffer);
}

bool CameraCoreMVCamera::set(int propId, double value) {
    CameraSdkStatus res;

    switch (propId) {
        case cv::CAP_PROP_GAMMA:
            res = CameraSetGamma(hCamera, (int) value);
            break;
        case cv::CAP_PROP_AUTO_EXPOSURE:
            res = CameraSetAeState(hCamera, (value == 0));  // 0 = auto, 1 = manual
            break;
        case cv::CAP_PROP_EXPOSURE:
            res = CameraSetExposureTime(hCamera, value);
            break;
        case cv::CAP_PROP_FPS:
            fps = (int) value;  // only store the value, doesn't influence the camera
            res = CAMERA_STATUS_SUCCESS;
            break;
        default:
            std::cerr << "CameraCoreMVCamera: setting " << propId << " is not implemented" << std::endl;
            return false;
    }

    if (res != CAMERA_STATUS_SUCCESS) {
        std::cerr << "CameraCoreMVCamera: setting property returned " << res << std::endl;
        return false;
    } else {
        return true;
    }
}

double CameraCoreMVCamera::get(int propId) const {
    CameraSdkStatus res;
    int intVal;
    double result;

    switch (propId) {
        case cv::CAP_PROP_FRAME_WIDTH:
        case cv::CAP_PROP_FRAME_HEIGHT: {
            tSdkImageResolution videoSize;
            res = CameraGetImageResolution(hCamera, &videoSize);
            result = (propId == cv::CAP_PROP_FRAME_WIDTH ? videoSize.iWidth : videoSize.iHeight);
        }
            break;
        case cv::CAP_PROP_GAMMA:
            res = CameraGetGamma(hCamera, &intVal);
            result = intVal;
            break;
        case cv::CAP_PROP_AUTO_EXPOSURE:
            res = CameraGetAeState(hCamera, &intVal);
            result = (intVal == TRUE ? 0 : 1);  // 0 = auto, 1 = manual
            break;
        case cv::CAP_PROP_EXPOSURE:
            res = CameraGetExposureTime(hCamera, &result);
            break;
        case cv::CAP_PROP_FPS:
            // res = CameraGetFrameRate(hCamera, &intVal);
            intVal = fps;  // only report the setting value
            res = CAMERA_STATUS_SUCCESS;
            result = intVal;
            break;
        default:
            std::cerr << "CameraCoreMVCamera: getting " << propId << " is not implemented" << std::endl;
            return false;
    }

    if (res != CAMERA_STATUS_SUCCESS) {
        std::cerr << "CameraCoreMVCamera: getting property returned " << res << std::endl;
        return 0;
    } else {
        return result;
    }
}

bool Camera::open(const package::ParamSet &params) {
    // Start the fetching thread
    if (th) {
        close();
    }
    threadShouldExit = false;
    th = new std::thread(&Camera::readFrameFromCamera, this, params);

    buffer[0] = cv::Mat(cv::Size(params.image_width(), params.image_height()), CV_8UC3);
    buffer[1] = cv::Mat(cv::Size(params.image_width(), params.image_height()), CV_8UC3);
    while (getFrame().empty()) std::this_thread::yield();

    return true;
}

Camera::~Camera() {
    if (th) {
        threadShouldExit = true;
        th->join();
        delete th;
        th = nullptr;
    }
}

void Camera::readFrameFromCamera(const package::ParamSet &params) {
    capInfoSS.str(std::string());  // clear capInfoSS

    delete cap;
    switch (params.camera_backend()) {
        case package::ParamSet_CameraBackend_OPENCV:
            cap = new CameraCoreOpenCV;
            break;
        case package::ParamSet_CameraBackend_MV_CAMERA:
            cap = new CameraCoreMVCamera;
            break;
    }

    // Open the camera in the same thread
    cap->open(params.camera_id(), cv::CAP_ANY);
    // cv::VideoCapture cap("v4l2src device=/dev/video0 io-mode=2 ! image/jpeg, width=(int)1280, height=(int)720 ! nvjpegdec ! video/x-raw ! videoconvert ! video/x-raw,format=BGR ! appsink", cv::CAP_GSTREAMER);
    if (!cap->isOpened()) {
        capInfoSS << "Failed to open camera " << params.camera_id() << "\n";
        std::cerr << capInfoSS.rdbuf();
        return;
    }

    // Set parameters
    if (!cap->set(cv::CAP_PROP_FRAME_WIDTH, params.image_width())) {
        capInfoSS << "Failed to set width.\n";
    }
    if (!cap->set(cv::CAP_PROP_FRAME_HEIGHT, params.image_height())) {
        capInfoSS << "Failed to set height.\n";
    }
    if (!cap->set(cv::CAP_PROP_FPS, params.fps())) {
        capInfoSS << "Failed to set fps.\n";
    }
    if (params.gamma().enabled()) {
        if (!cap->set(cv::CAP_PROP_GAMMA, params.gamma().val())) {
            capInfoSS << "Failed to set gamma.\n";
        }
    }
    if (params.manual_exposure().enabled()) {
        if (!cap->set(cv::CAP_PROP_AUTO_EXPOSURE, 1)) {  // manual
            capInfoSS << "Failed to set manual exposure.\n";
        }
        if (!cap->set(cv::CAP_PROP_EXPOSURE, params.manual_exposure().val())) {
            capInfoSS << "Failed to set exposure.\n";
        }
    } else {
        if (!cap->set(cv::CAP_PROP_AUTO_EXPOSURE, 0)) {  // auto
            capInfoSS << "Failed to set auto exposure.\n";
        }
    }

    // Get a test frame
    cap->read(buffer[0]);
    if (buffer->empty()) {
        capInfoSS << "Failed to fetch test image from camera " << params.camera_id() << "\n";
        std::cerr << capInfoSS.rdbuf();
        return;
    }
    if (buffer[0].cols != params.image_width() || buffer[0].rows != params.image_height()) {
        capInfoSS << "Invalid frame size. "
                  << "Expected: " << params.image_width() << "x" << params.image_height() << ", "
                  << "Actual: " << buffer[0].cols << "x" << buffer[0].rows << "\n";
        std::cerr << capInfoSS.rdbuf();
        return;
    }

    // Report actual parameters
    capInfoSS << "Camera " << params.camera_id() << ", "
              << cap->get(cv::CAP_PROP_FRAME_WIDTH) << "x" << cap->get(cv::CAP_PROP_FRAME_HEIGHT)
              << " @ " << cap->get(cv::CAP_PROP_FPS) << " fps\n"
              << "Gamma: " << cap->get(cv::CAP_PROP_GAMMA) << "\n"
              << "Auto Exposure: " << cap->get(cv::CAP_PROP_AUTO_EXPOSURE) << "\n"
              << "Exposure: " << cap->get(cv::CAP_PROP_EXPOSURE) << "\n";
    std::cout << capInfoSS.rdbuf();

    while (true) {

        uint8_t workingBuffer = 1 - lastBuffer;

        if (threadShouldExit || !cap->isOpened()) {
            bufferCaptureTime[workingBuffer] = TimePoint();  // indicate invalid frame
            lastBuffer = workingBuffer;  // switch
            break;
        }

        if (!cap->read(buffer[workingBuffer])) {
            continue;  // try again
        }

        // Save frame if required
        videoWriterMutex.lock();
        {
            if (videoWriter.isOpened()) videoWriter << buffer[workingBuffer];
        }
        videoWriterMutex.unlock();

        bufferCaptureTime[workingBuffer] = std::chrono::steady_clock::now();

        lastBuffer = workingBuffer;

        ++cumulativeFrameCounter;  // the only place of incrementing
    }

    cap->release();
    delete cap;
    cap = nullptr;
    std::cout << "Camera: closed\n";
}

void Camera::close() {
    if (th) {
        threadShouldExit = true;
        th->join();  // cap is managed in the thread
        delete th;
        th = nullptr;
    }
}

bool Camera::startRecordToVideo(const std::string &filename, const cv::Size &size) {
    bool ret;
    videoWriterMutex.lock();
    {
        if (videoWriter.isOpened()) videoWriter.release();
        videoWriter.open(filename, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), getFPS(), size);
        ret = videoWriter.isOpened();
    }
    videoWriterMutex.unlock();
    return ret;
}

void Camera::stopRecordToVideo() {
    videoWriterMutex.lock();
    {
        videoWriter.release();
    }
    videoWriterMutex.unlock();
}

bool Camera::isRecordingVideo() {
    bool ret;
    videoWriterMutex.lock();
    {
        ret = videoWriter.isOpened();
    }
    videoWriterMutex.unlock();
    return ret;
}

}