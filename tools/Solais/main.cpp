//
// Created by liuzikai on 3/11/21.
//

#include "Camera.h"
#include "Executor.h"
#include "ArmorDetector.h"
#include "ImageDataManager.h"
#include "TerminalSocket.h"
#include "TerminalParameters.h"
#include "Parameters.pb.h"
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace meta;
using namespace package;

ParamSet params;

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

string previewSource = "";

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

    // Send preview image in an individual package, which will trigger next-cycle fetching
    resultPackage.Clear();
    if (previewSource == "Camera") {
        resultPackage.set_allocated_camera_image(allocateProtoJPEGImage(executor->getCamera()->getFrame()));
        socketServer.sendBytes("res", resultPackage);
    } else if (!previewSource.empty()) {
        resultPackage.set_allocated_camera_image(allocateProtoJPEGImage(executor->getDataManager()->getImage(previewSource)));
        socketServer.sendBytes("res", resultPackage);
        previewSource = "";  // image only needs to be sent once
    }

    // The remaining images follows the input image package, which forms pipeline with the previous package
    if (executor->getCurrentAction() != Executor::NONE || forceSendResult) {
        resultPackage.Clear();

        // Empty handled in allocateProtoJPEGImage
        // FIXME: lock required?
        resultPackage.set_allocated_brightness_image(allocateProtoJPEGImage(executor->getDetector()->brightnessImage()));
        resultPackage.set_allocated_color_image(allocateProtoJPEGImage(executor->getDetector()->colorImage()));
        resultPackage.set_allocated_contour_image(allocateProtoJPEGImage(executor->getDetector()->contourImage()));
        resultPackage.set_allocated_armor_image(allocateProtoJPEGImage(executor->getDetector()->armorImage()));

        socketServer.sendBytes("res", resultPackage);
    }
}

void handleRecvSingleString(std::string_view name, std::string_view s) {
    if (name == "loadDataSet") {

        if (executor->loadDataSet(string(s)) == 0) {
            sendStatusBarMsg("Failed to load data set " + string(s));
        } else {
            socketServer.sendListOfStrings("imageList", executor->getDataManager()->getImageNames());
            sendStatusBarMsg("Data set " + string(s) + " loaded");
        }

    } else if (name == "runImage") {

        executor->startSingleImageDetection(string(s));  // blocking
        sendStatusBarMsg("Run on image " + string(s));
        sendResult(true);  // always send result

    } else if (name == "viewImage") {

        previewSource = s;

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
            params = recvParams;
            executor->applyParams(params);
            sendStatusBarMsg("Parameters applied");
        }

    } else if (name == "getParams") {

        socketServer.sendBytes("params", params);

    } else if (name == "stop") {

        executor->stop();

    } else if (name == "runCamera") {

        if (!executor->startRealTimeDetection()) {
            sendStatusBarMsg("Failed to set executor action REAL_TIME_DETECTION");
        }

    } else if (name == "viewCamera") {

        previewSource = "Camera";

    } else {

        std::cerr << "Unknown bytes package <" << name << ">" << std::endl;

    }
}

// Should not operates on these components directly
std::unique_ptr<Camera> camera;
std::unique_ptr<ArmorDetector> detector;
std::unique_ptr<ImageDataManager> dataManager;

int main(int argc, char *argv[]) {

    camera = std::make_unique<Camera>();
    detector = std::make_unique<ArmorDetector>();
    dataManager = std::make_unique<ImageDataManager>();
    executor = std::make_unique<Executor>(camera.get(), detector.get(), dataManager.get());

    params.set_camera_id(0);
    params.set_image_width(1280);
    params.set_image_height(720);
    params.set_fps(120);
    params.set_allocated_gamma(allocToggledDouble(false));

    params.set_enemy_color(ParamSet::BLUE);
    params.set_brightness_threshold(155);
    params.set_color_threshold_mode(ParamSet::RB_CHANNELS);
    params.set_allocated_hsv_red_hue(allocDoubleRange(150, 30));  // across the 0 (180) point
    params.set_allocated_hsv_blue_hue(allocDoubleRange(90, 150));
    params.set_rb_channel_threshold(55);
    params.set_allocated_color_dilate(allocToggledInt(true, 6));

    params.set_contour_fit_function(ParamSet::ELLIPSE);
    params.set_allocated_contour_pixel_count(allocToggledDouble(true, 15));
    params.set_allocated_contour_min_area(allocToggledDouble(false, 3));
    params.set_allocated_long_edge_min_length(allocToggledInt(true, 30));
    params.set_allocated_light_aspect_ratio(allocToggledDoubleRange(true, 2, 30));

    params.set_allocated_light_length_max_ratio(allocToggledDouble(true, 1.5));
    params.set_allocated_light_x_dist_over_l(allocToggledDoubleRange(false, 1, 3));
    params.set_allocated_light_y_dist_over_l(allocToggledDoubleRange(false, 0, 1));
    params.set_allocated_light_angle_max_diff(allocToggledDouble(true, 10));
    params.set_allocated_armor_aspect_ratio(allocToggledDoubleRange(true, 1.25, 5));

    executor->applyParams(params);

    socketServer.startAccept();
    socketServer.setCallbacks(handleRecvSingleString,
                              nullptr,
                              handleRecvBytes,
                              nullptr);

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard(ioContext.get_executor());
    ioContext.run();  // this operation is blocking, until ioContext is deleted

    return 0;
}