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

    } else if (name == "fps") {

        socketServer.sendSingleInt("fps", executor->fetchAndClearFrameCounter());

    } else if (name == "stop") {

            if (!executor->setAction(Executor::NONE)) {
                socketServer.sendSingleString("msg", "Failed to set executor action NONE");
            }

    } else if (name == "runCamera") {

        if (!executor->setAction(Executor::REAL_TIME_DETECTION)) {
            socketServer.sendSingleString("msg", "Failed to set executor action REAL_TIME_DETECTION");
        }

    } else {

        std::cerr << "Unknown bytes package <" << name << ">" << std::endl;

    }
}

int main(int argc, char *argv[]) {

    camera = std::make_unique<Camera>();
    detector = std::make_unique<ArmorDetector>();
    executor = std::make_unique<Executor>(camera.get(), detector.get());

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