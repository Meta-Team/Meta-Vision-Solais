//
// Created by liuzikai on 3/11/21.
//

#include "Executor.h"  // includes headers of all components
#include "TerminalSocket.h"
#include "TerminalParameters.h"
#include "Parameters.pb.h"
#include <iostream>
#include <thread>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/core/cuda.hpp>

using namespace meta;
using namespace package;

/** TCP IO **/

boost::asio::io_context tcpIOContext;
std::thread *tcpIOThread = nullptr;

// Setup a server with automatic acceptance
TerminalSocketServer socketServer(tcpIOContext, 8800, [](auto s) {
    std::cout << "TerminalSocketServer: disconnected" << std::endl;
    s->startAccept();
});


/** TCP Handling **/

// Client should only operates on the executor
std::unique_ptr<Executor> executor;

// Reuse message objects: developers.google.com/protocol-buffers/docs/cpptutorial#optimization-tips
ParamSet recvParams;
Result resultPackage;

Image *allocProtoJPG(const cv::Mat &mat) {
    auto image = new package::Image;
    image->set_format(package::Image::JPEG);

    if (!mat.empty()) {
        cv::Mat outImage;
        std::vector<uchar> buf;
        float ratio = (float) TERMINAL_IMAGE_PREVIEW_HEIGHT / (float) mat.rows;

        cv::resize(mat, outImage, cv::Size(), ratio, ratio);
        cv::imencode(".jpg", outImage, buf, {cv::IMWRITE_JPEG_QUALITY, 60});

        image->set_data(buf.data(), buf.size());
    }
    return image;
}

void sendStatusBarMsg(const std::string &msg) {
    socketServer.sendSingleString("msg", "Core: " + msg);
}

void sendResult() {
    // Always send a package, but non-empty only if the executor is running
    if (executor->getCurrentAction() != Executor::NONE) {
        resultPackage.Clear();

        // Detector images
        {
            executor->detectorOutputMutex().lock();
            // If can't lock immediately, simply wait. Detector only performs several non-copy assignments.
            {
                // Empty handled in allocateProtoJPEGImage
                resultPackage.set_allocated_camera_image(allocProtoJPG(executor->detector()->originalImage()));
                resultPackage.set_allocated_brightness_image(allocProtoJPG(executor->detector()->brightnessImage()));
                resultPackage.set_allocated_color_image(allocProtoJPG(executor->detector()->colorImage()));
                resultPackage.set_allocated_contour_image(allocProtoJPG(executor->detector()->contourImage()));
            }
            executor->detectorOutputMutex().unlock();
        }

        // Armors
        {
            executor->armorsOutputMutex.lock();
            // If can't lock immediately, simply wait. Executor only performs a copy.
            {
                float imageScale =
                        (float) TERMINAL_IMAGE_PREVIEW_HEIGHT / executor->getCurrentParams().image_height();
                for (const auto &armor : executor->armorsOutput()) {
                    auto armorInfo = resultPackage.add_armors();
                    for (int i = 0; i < 4; i++) {
                        auto imagePoint = armorInfo->add_image_points();
                        imagePoint->set_x(armor.imgPoints[i].x * imageScale);
                        imagePoint->set_y(armor.imgPoints[i].y * imageScale);
                    }
                    armorInfo->set_allocated_image_center(allocResultPoint2f(
                            armor.imgCenter.x * imageScale, armor.imgCenter.y * imageScale));
                    armorInfo->set_allocated_offset(allocResultPoint3f(armor.offset.x, armor.offset.y, armor.offset.z));
                    armorInfo->set_allocated_rotation(
                            allocResultPoint3f(armor.rotation.x, armor.rotation.y, armor.rotation.z));
                    armorInfo->set_large_armor(armor.largeArmor);
                    armorInfo->set_number(armor.number);
                }
            }
            executor->armorsOutputMutex.unlock();
        }

        // Aiming
        {
            auto command = executor->aimingSolver()->getControlCommand();
            resultPackage.set_aiming_info("Yaw: " + std::to_string(command.yawDelta) + "\n" +
                                          "Pitch: " + std::to_string(command.pitchDelta));
        }
        socketServer.sendBytes("res", resultPackage);

        if (executor->getCurrentAction() == meta::Executor::SINGLE_IMAGE_DETECTION) {
            executor->stop();
        }
    } else {
        socketServer.sendBytes("res", nullptr, 0);
    }

}

void handleRecvSingleString(std::string_view name, std::string_view s) {
    if (name == "switchImageSet") {
        if (executor->switchImageSet(std::string(s)) == 0) {
            sendStatusBarMsg("empty data set " + std::string(s));
        } else {
            socketServer.sendListOfStrings("imageList", executor->imageSet()->getImageList());
            sendStatusBarMsg("image set \"" + std::string(s) + "\" loaded");
        }

    } else if (name == "runImage") {
        executor->startSingleImageDetection(std::string(s));  // blocking
        socketServer.sendSingleString("executionStarted", "image " + std::string(s));  // let terminal fetch

    } else if (name == "switchParamSet") {
        executor->switchParamSet(std::string(s));
        sendStatusBarMsg("Switched to parameter set \"" + std::string(s) + "\"");

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
                std::to_string(executor->fetchAndClearSerialFrameCounter()),
        });

    } else if (name == "setParams") {
        if (!recvParams.ParseFromArray(buf, size)) {
            sendStatusBarMsg("invalid ParamSet package");
        } else {
            executor->saveAndApplyParams(recvParams);
            sendStatusBarMsg(
                    "parameter set \"" + executor->dataManager()->currentParamSetName() + "\" saved and applied");
        }

    } else if (name == "getParams") {
        socketServer.sendBytes("params", executor->getCurrentParams());

    } else if (name == "getCurrentParamSetName") {
        socketServer.sendSingleString("currentParamSetName", executor->dataManager()->currentParamSetName());

    } else if (name == "stop") {
        executor->stop();

    } else if (name == "runCamera") {
        if (!executor->startRealTimeDetection()) {
            sendStatusBarMsg("failed to start real time detection on camera");
        } else {
            socketServer.sendSingleString("executionStarted", "camera");

            // Send camera info
            resultPackage.Clear();
            resultPackage.set_camera_info(executor->camera()->getCapInfo());
            socketServer.sendBytes("res", resultPackage);
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

    } else if (name == "captureImage") {
        std::string filename = executor->captureImageFromCamera();
        sendStatusBarMsg("capture as " + filename);

    } else {
        std::cerr << "Unknown bytes package <" << name << ">" << std::endl;
    }
}

/** Serial IO **/
boost::asio::io_context serialIOContext;

/** Components **/

// TCP handling should not operates on these components directly, so they are put at last
std::unique_ptr<Camera> camera;
std::unique_ptr<ImageSet> imageSet;
std::unique_ptr<ArmorDetector> detector;
std::unique_ptr<ParamSetManager> paramSetManager;
std::unique_ptr<PositionCalculator> positionCalculator;
std::unique_ptr<AimingSolver> aimingSolver;
std::unique_ptr<Serial> serial;

int main(int argc, char *argv[]) {

    std::cout << "CUDA device count: " << cv::cuda::getCudaEnabledDeviceCount() << std::endl;
    std::cout << cv::getBuildInformation() << std::endl;

    camera = std::make_unique<Camera>();
    imageSet = std::make_unique<ImageSet>();
    detector = std::make_unique<ArmorDetector>();
    paramSetManager = std::make_unique<ParamSetManager>();
    positionCalculator = std::make_unique<PositionCalculator>();
    aimingSolver = std::make_unique<AimingSolver>();
    if (strlen(SERIAL_DEVICE) != 0) {
        serial = std::make_unique<Serial>(serialIOContext);
    } else {
        std::cerr << "Serial disabled for debug purpose" << std::endl;
    }
    executor = std::make_unique<Executor>(camera.get(), imageSet.get(), detector.get(), paramSetManager.get(),
                                          positionCalculator.get(), aimingSolver.get(), serial.get());

    tcpIOThread = new std::thread([] {
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard(tcpIOContext.get_executor());
        tcpIOContext.run();  // this operation is blocking, until ioContext is deleted
    });

    socketServer.startAccept();
    socketServer.setCallbacks(handleRecvSingleString,
                              nullptr,
                              handleRecvBytes,
                              nullptr);

    // Start on camera
    executor->startRealTimeDetection();

    // Main thread is then used for serial IO
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard(serialIOContext.get_executor());
    serialIOContext.run();  // this operation is blocking, until ioContext is deleted

    // Shall not get to here
    tcpIOThread->join();

    return 0;
}