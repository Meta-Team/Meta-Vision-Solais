//
// Created by liuzikai on 3/11/21.
//

#include "Camera.h"
#include "ImageSet.h"
#include "Executor.h"
#include "ArmorDetector.h"
#include "ParamSetManager.h"
#include "TerminalSocket.h"
#include "TerminalParameters.h"
#include "Parameters.pb.h"
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/core/cuda.hpp>

using namespace meta;
using namespace package;

// Client should only operates on the executor
std::unique_ptr<Executor> executor;

boost::asio::io_context ioContext;

// Setup a server with automatic acceptance
TerminalSocketServer socketServer(ioContext, 8800, [](auto s) {
    std::cout << "TerminalSocketServer: disconnected" << std::endl;
    s->startAccept();
});

// Reuse message objects: developers.google.com/protocol-buffers/docs/cpptutorial#optimization-tips
ParamSet recvParams;
Result resultPackage;

Image *allocateProtoJPEGImage(const cv::Mat &mat) {
    auto image = new package::Image;
    image->set_format(package::Image::JPEG);

    if (!mat.empty()) {
        cv::Mat outImage;
        std::vector<uchar> buf;
        float ratio = (float) TERMINAL_IMAGE_PREVIEW_HEIGHT / (float) mat.rows;

        cv::resize(mat, outImage, cv::Size(), ratio, ratio);
        cv::imencode(".jpg", outImage, buf, {cv::IMWRITE_JPEG_QUALITY, 80});

        image->set_data(buf.data(), buf.size());
    }
    return image;
}

void sendStatusBarMsg(const string &msg) {
    socketServer.sendSingleString("msg", "Core: " + msg);
}

void sendResult() {
    // Always send a package, but non-empty only if the executor is running
    resultPackage.Clear();
    if (executor->getCurrentAction() != Executor::NONE) {
        executor->detectorOutputMutex().lock();
        // If can't lock immediately, simply wait. Detector only performs several non-copy assignments
        {
            // Empty handled in allocateProtoJPEGImage
            resultPackage.set_allocated_camera_image(allocateProtoJPEGImage(executor->detector()->originalImage()));
            resultPackage.set_allocated_brightness_image(allocateProtoJPEGImage(executor->detector()->brightnessImage()));
            resultPackage.set_allocated_color_image(allocateProtoJPEGImage(executor->detector()->colorImage()));
            resultPackage.set_allocated_contour_image(allocateProtoJPEGImage(executor->detector()->contourImage()));
            resultPackage.set_allocated_armor_image(allocateProtoJPEGImage(executor->detector()->armorImage()));
        }
        executor->detectorOutputMutex().unlock();
        if (executor->getCurrentAction() == meta::Executor::SINGLE_IMAGE_DETECTION) {
            executor->stop();
        }
    }
    socketServer.sendBytes("res", resultPackage);
}

void handleRecvSingleString(std::string_view name, std::string_view s) {
    if (name == "switchImageSet") {
        if (executor->switchImageSet(string(s)) == 0) {
            sendStatusBarMsg("Failed to load data set " + string(s));
        } else {
            socketServer.sendListOfStrings("imageList", executor->imageSet()->getImageList());
            sendStatusBarMsg("image set \"" + string(s) + "\" loaded");
        }

    } else if (name == "runImage") {
        executor->startSingleImageDetection(string(s));  // blocking
        socketServer.sendSingleString("executionStarted", "image " + string(s));  // let terminal fetch

    } else if (name == "switchParamSet") {
        executor->switchParamSet(string(s));
        sendStatusBarMsg("Switched to parameter set \"" + string(s) + "\"");

    } else goto INVALID_COMMAND;

    return;
    INVALID_COMMAND:
    std::cerr << "Invalid single-string package <" << name << "> \"" << s << "\"" << std::endl;
}

void handleRecvBytes(std::string_view name, const uint8_t *buf, size_t size) {
    if (name == "fetch") {
        sendResult();  // reply anyway, whether the executor is running or not

    } else if (name == "fps") {
        socketServer.sendListOfStrings("fps", {
            std::to_string(executor->fetchAndClearInputFrameCounter()),
            std::to_string(executor->fetchAndClearExecutorFrameCounter()),
        });

    }  else if (name == "setParams") {
        if (!recvParams.ParseFromArray(buf, size)) {
            sendStatusBarMsg("invalid ParamSet package");
        } else {
            executor->saveAndApplyParams(recvParams);
            sendStatusBarMsg("parameter set \"" + executor->dataManager()->currentParamSetName() + "\" saved and applied");
        }

    } else if (name == "getParams") {
        socketServer.sendBytes("params", executor->getCurrentParams());

    } else if (name == "getCurrentParamSetName") {
        socketServer.sendSingleString("currentParamSetName", executor->dataManager()->currentParamSetName());

    }else if (name == "stop") {
        executor->stop();

    } else if (name == "runCamera") {
        if (!executor->startRealTimeDetection()) {
            sendStatusBarMsg("failed to start real time detection on camera");
        } else {
            socketServer.sendSingleString("executionStarted", "camera");
        }

    } else if (name == "runImageSet") {
        if (!executor->startImageSetDetection()) {
            sendStatusBarMsg("failed to start streaming on image set");
        } else {
            socketServer.sendSingleString("executionStarted", "image set");
        }

    } else if (name == "fetchLists") {
        socketServer.sendListOfStrings("imageSetList", executor->imageSet()->getImageSetList());
        socketServer.sendListOfStrings("paramSetList", executor->dataManager()->getParamSetList());
        socketServer.sendSingleString("currentParamSetName", executor->dataManager()->currentParamSetName());
        socketServer.sendBytes("params", executor->getCurrentParams());

    } else if (name == "reloadLists") {
        executor->reloadLists();  // switch to default parameter set

    } else {
        std::cerr << "Unknown bytes package <" << name << ">" << std::endl;
    }
}

// Should not operates on these components directly
std::unique_ptr<Camera> camera;
std::unique_ptr<ImageSet> imageSet;
std::unique_ptr<ArmorDetector> detector;
std::unique_ptr<ParamSetManager> paramSetManager;

int main(int argc, char *argv[]) {

    std::cout << "CUDA device count: "<< cv::cuda::getCudaEnabledDeviceCount() << std::endl;
    std::cout << cv::getBuildInformation() << std::endl;

    camera = std::make_unique<Camera>();
    imageSet = std::make_unique<ImageSet>();
    detector = std::make_unique<ArmorDetector>();
    paramSetManager = std::make_unique<ParamSetManager>();
    executor = std::make_unique<Executor>(camera.get(), imageSet.get(), detector.get(), paramSetManager.get());

    socketServer.startAccept();
    socketServer.setCallbacks(handleRecvSingleString,
                              nullptr,
                              handleRecvBytes,
                              nullptr);

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard(ioContext.get_executor());
    ioContext.run();  // this operation is blocking, until ioContext is deleted

    return 0;
}