//
// Created by liuzikai on 3/11/21.
//

#include "Camera.h"
#include "Executor.h"
#include "ArmorDetector.h"
#include "DataManager.h"
#include "TerminalSocket.h"
#include "TerminalParameters.h"
#include "Parameters.pb.h"
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

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

void sendResult(bool forceSendResult = false) {
    if (executor->getCurrentAction() != Executor::NONE || forceSendResult) {

        // Send preview image in an individual package, which will trigger next-cycle fetching
        resultPackage.Clear();
        resultPackage.set_allocated_camera_image(allocateProtoJPEGImage(executor->detector()->originalImage()));
        socketServer.sendBytes("res", resultPackage);

        // The remaining images follows the input image package, which forms pipeline with the previous package
        resultPackage.Clear();

        // Empty handled in allocateProtoJPEGImage
        // FIXME: lock required?
        resultPackage.set_allocated_brightness_image(allocateProtoJPEGImage(executor->detector()->brightnessImage()));
        resultPackage.set_allocated_color_image(allocateProtoJPEGImage(executor->detector()->colorImage()));
        resultPackage.set_allocated_contour_image(allocateProtoJPEGImage(executor->detector()->contourImage()));
        resultPackage.set_allocated_armor_image(allocateProtoJPEGImage(executor->detector()->armorImage()));

        socketServer.sendBytes("res", resultPackage);
    }
}

void handleRecvSingleString(std::string_view name, std::string_view s) {
    if (name == "loadImageDataSet") {
        if (executor->loadImageDataSet(string(s)) == 0) {
            sendStatusBarMsg("Failed to load data set " + string(s));
        } else {
            socketServer.sendListOfStrings("imageList", executor->dataManager()->getImageList());
            sendStatusBarMsg("Data set \"" + string(s) + "\" loaded");
        }

    } else if (name == "runImage") {
        executor->startSingleImageDetection(string(s));  // blocking
        sendStatusBarMsg("Run on image " + string(s));
        sendResult(true);  // always send result

    } else if (name == "switchParams") {
        executor->switchParams(string(s));
        sendStatusBarMsg("Switched to parameter set \"" + string(s) + "\"");

    } else goto INVALID_COMMAND;

    return;
    INVALID_COMMAND:
    std::cerr << "Invalid single-string package <" << name << "> \"" << s << "\"" << std::endl;
}

void handleRecvBytes(std::string_view name, const uint8_t *buf, size_t size) {
    if (name == "fetch") {
        sendResult();

    } else if (name == "fps") {
        socketServer.sendSingleInt("fps", executor->fetchAndClearFrameCounter());

    } else if (name == "setParams") {
        if (!recvParams.ParseFromArray(buf, size)) {
            sendStatusBarMsg("Invalid ParamSet package");
        } else {
            executor->saveAndApplyParams(recvParams);
            sendStatusBarMsg("Parameter set \"" + executor->dataManager()->currentParamSetName() + "\" saved and applied");
        }

    } else if (name == "getParams") {
        socketServer.sendBytes("params", executor->getCurrentParams());

    } else if (name == "getCurrentParamSetName") {
        socketServer.sendSingleString("currentParamSetName", executor->dataManager()->currentParamSetName());

    }else if (name == "stop") {
        executor->stop();

    } else if (name == "runCamera") {
        if (!executor->startRealTimeDetection()) {
            sendStatusBarMsg("Failed to set executor action REAL_TIME_DETECTION");
        } else {
            sendStatusBarMsg("Start real time detection");
        }

    } else if (name == "fetchLists") {
        socketServer.sendListOfStrings("dataSetList", executor->dataManager()->getDataSetList());
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
std::unique_ptr<ArmorDetector> detector;
std::unique_ptr<DataManager> dataManager;

int main(int argc, char *argv[]) {

    camera = std::make_unique<Camera>();
    detector = std::make_unique<ArmorDetector>();
    dataManager = std::make_unique<DataManager>();
    executor = std::make_unique<Executor>(camera.get(), detector.get(), dataManager.get());

    socketServer.startAccept();
    socketServer.setCallbacks(handleRecvSingleString,
                              nullptr,
                              handleRecvBytes,
                              nullptr);

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard(ioContext.get_executor());
    ioContext.run();  // this operation is blocking, until ioContext is deleted

    return 0;
}