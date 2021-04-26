//
// Created by liuzikai on 3/11/21.
//

#include "Camera.h"
#include "Executor.h"
#include "ArmorDetector.h"
//#include "DetectorTuner.h"
#include "TerminalSocket.h"
#include "TerminalParameters.h"
#include "Parameters.pb.h"
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace meta;
using namespace package;

ParamSet params;

std::unique_ptr<Camera> camera;
std::unique_ptr<ArmorDetector> detector;
std::unique_ptr<Executor> executor;

boost::asio::io_context ioContext;

// Setup a server with automatic acceptance
TerminalSocketServer socketServer(ioContext, 8800, [](auto s) {
    std::cout << "TerminalSocketServer: disconnected" << std::endl;
    s->startAccept();
});

// Reuse message objects: developers.google.com/protocol-buffers/docs/cpptutorial#optimization-tips
Result resultPackage;

Image *allocateProtoJPEGImage(const cv::Mat &mat) {
    cv::Mat outImage;
    std::vector<uchar> buf;
    float ratio = (float) TERMINAL_IMAGE_PREVIEW_HEIGHT / (float) mat.rows;

    cv::resize(mat, outImage, cv::Size(), ratio, ratio);
    cv::imencode(".jpg", outImage, buf, {cv::IMWRITE_JPEG_QUALITY, 80});

    auto image = new package::Image;
    image->set_format(package::Image::JPEG);
    image->set_data(buf.data(), buf.size());
    return image;
}

void handleRecvSingleString(std::string_view name, std::string_view s) {
    if (name == "") {

    } else goto INVALID_COMMAND;

    return;
    INVALID_COMMAND:
    std::cerr << "Invalid single-string package <" << name << "> \"" << s << "\"" << std::endl;
}

void handleRecvBytes(std::string_view name, const uint8_t *buf, size_t size) {
    if (name == "fetch") {

        // Send camera image
        resultPackage.Clear();
        resultPackage.set_allocated_camera_image(allocateProtoJPEGImage(camera->getFrame()));
        socketServer.sendBytes("res", resultPackage);

        if (executor->getCurrentAction() != Executor::NONE) {
            // Send processing image
            resultPackage.Clear();
            resultPackage.set_allocated_brightness_threshold_image(allocateProtoJPEGImage(detector->getImgBrightnessThreshold()));
            // TODO: more image to send
            socketServer.sendBytes("res", resultPackage);
        }

    } else if (name == "stop") {

        executor->setAction(Executor::NONE);

    } else if (name == "runCamera") {

        executor->setAction(Executor::REAL_TIME_DETECTION);

    } else {

        std::cerr << "Unknown bytes package <" << name << ">" << std::endl;

    }
}

int main(int argc, char *argv[]) {

    params.set_camera_id(0);
    params.set_image_width(1280);
    params.set_image_height(720);
    params.set_fps(120);
    auto gamma = new ToggledDouble;
    gamma->set_enabled(false);
    params.set_allocated_gamma(gamma);
    camera->open(params);

    socketServer.startAccept();
    socketServer.setCallbacks(handleRecvSingleString,
                              nullptr,
                              nullptr,
                              nullptr);

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard(ioContext.get_executor());
    ioContext.run();  // this operation is blocking, until ioContext is deleted

    return 0;
}