#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QtWidgets/QLabel>
#include <QTextStream>
#include <iostream>

namespace meta {

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow),
        tuner(&detector, &camera) {

    ui->setupUi(this);

    bindings = {

            // ================================ General ================================

            new ValueCheckSpinBinding<int>(nullptr, &sharedParams.imageWidth,
                                           nullptr, ui->imageWidthSpin),

            new ValueCheckSpinBinding<int>(nullptr, &sharedParams.imageHeight,
                                           nullptr, ui->imageHeightSpin),

            // ================================ Camera ================================

            new ValueCheckSpinBinding<int>(nullptr, &cameraParams.fps,
                                           nullptr, ui->frameRateSpin),

            new ValueCheckSpinBinding<int>(nullptr, &cameraParams.cameraID,
                                           nullptr, ui->cameraIDSpin),

            new ValueCheckSpinBinding<double>(&cameraParams.enableGamma, &cameraParams.gamma,
                                              ui->gammaCheck, ui->gammaSpin),

            // ================================ Brightness Filter ================================

            new ValueCheckSpinBinding<double>(
                    nullptr, &detectorParams.brightnessThreshold,
                    nullptr, ui->brightnessSpin),

            // ================================ Color Filter ================================

            new EnumRadioBinding<ArmorDetector::ColorThresholdMode>(
                    &detectorParams.colorThresholdMode,
                    std::vector<std::pair<ArmorDetector::ColorThresholdMode, QRadioButton *>>{
                            {ArmorDetector::HSV,         ui->hsvRadio},
                            {ArmorDetector::RB_CHANNELS, ui->rbChannelRadio},
                    }),
            new RangeCheckSpinBinding<double>(
                    nullptr, &detectorParams.hsvRedHue,
                    nullptr, ui->hsvRedHueMinSpin, ui->hsvRedHueMaxSpin),
            new RangeCheckSpinBinding<double>(
                    nullptr, &detectorParams.hsvBlueHue,
                    nullptr, ui->hsvBlueHueMinSpin, ui->hsvBlueHueMaxSpin),
            new ValueCheckSpinBinding<double>(
                    nullptr, &detectorParams.rbChannelThreshold,
                    nullptr, ui->rbChannelThresholdSpin),
            new ValueCheckSpinBinding<int>(
                    &detectorParams.enableColorDilate, &detectorParams.colorDilate,
                    ui->colorDliateCheck, ui->colorDliateSpin),

            // ================================ Contour Filter ================================

            new EnumRadioBinding<ArmorDetector::ContourFitFunction>(
                    &detectorParams.contourFitFunction,
                    std::vector<std::pair<ArmorDetector::ContourFitFunction, QRadioButton *>>{
                            {ArmorDetector::MIN_AREA_RECT, ui->minAreaRectRadio},
                            {ArmorDetector::ELLIPSE,       ui->fitEllipseRadio},
                    }),
            new ValueCheckSpinBinding<double>(
                    &detectorParams.filterContourPixelCount, &detectorParams.contourPixelCount,
                    ui->contourPixelMinCountCheck, ui->contourPixelMinCountSpin),
            new ValueCheckSpinBinding<double>(
                    &detectorParams.filterContourMinArea, &detectorParams.contourMinArea,
                    ui->contourMinAreaCheck, ui->contourMinAreaSpin),
            new ValueCheckSpinBinding<int>(
                    &detectorParams.filterLongEdgeMinLength, &detectorParams.longEdgeMinLength,
                    ui->longEdgeMinLengthCheck, ui->longEdgeMinLengthSpin),
            new RangeCheckSpinBinding<double>(&detectorParams.filterLightAspectRatio, &detectorParams.lightAspectRatio,
                                              ui->aspectRatioFilterCheck, ui->aspectRatioMinSpin,
                                              ui->aspectRatioMaxSpin),

            // ================================ Armor Filter ================================

            new ValueCheckSpinBinding<double>(&detectorParams.filterLightLengthRatio,
                                              &detectorParams.lightLengthMaxRatio,
                                              ui->lightLengthRatioCheck, ui->lightLengthRatioMaxSpin),
            new RangeCheckSpinBinding<double>(&detectorParams.filterLightXDistance, &detectorParams.lightXDistOverL,
                                              ui->lightXDiffCheck, ui->lightXDiffMinSpin, ui->lightXDiffMaxSpin),

            new RangeCheckSpinBinding<double>(&detectorParams.filterLightYDistance, &detectorParams.lightYDistOverL,
                                              ui->lightYDiffCheck, ui->lightYDiffMinSpin, ui->lightYDiffMaxSpin),
            new ValueCheckSpinBinding<double>(&detectorParams.filterLightAngleDiff, &detectorParams.lightAngleMaxDiff,
                                              ui->lightAngleDiffCheck, ui->lightAngleDiffMaxSpin),
            new RangeCheckSpinBinding<double>(&detectorParams.filterArmorAspectRatio, &detectorParams.armorAspectRatio,
                                              ui->armorAspectRatioCheck, ui->armorAspectRatioMinSpin,
                                              ui->armorAspectRatioMaxSpin),
    };

    camera.registerNewFrameCallBack(&MainWindow::updateCameraFrame, this);

    updateUIFromParams();

    connect(ui->runSingleImageButton, &QPushButton::clicked, this, &MainWindow::runSingleDetectionOnImage);
    connect(ui->loadDataSetButton, &QPushButton::clicked, this, &MainWindow::loadSelectedDataSet);
    connect(ui->switchCameraButton, &QPushButton::clicked, this, &MainWindow::switchCamera);

    ui->contourImageWidget->installEventFilter(this);

    socketClient.setCallbacks(this, nullptr, nullptr, MainWindow::handleRecvBytes, nullptr);
}

void MainWindow::handleClientDisconnection(TerminalSocketClient *client) {
    std::cout << "TerminalSocketClient: disconnected" << std::endl;
    client->connect("127.0.0.1", "8800", MainWindow::handleClientDisconnection);
}

void MainWindow::handleRecvBytes(void *ptr, const char *name, const uint8_t *buf, size_t size) {
    auto inst = static_cast<MainWindow *>(ptr);
    if (strcmp(name, "camera") == 0) {
        QPixmap pixmap;
        pixmap.loadFromData(buf, size);
        inst->ui->cameraImage->setPixmap(pixmap.scaledToHeight(inst->ui->cameraImage->height(), Qt::SmoothTransformation));

    } else {
        std::cerr << "Unknown Bytes package named \"" << name << "\"" << std::endl;
    }
}

MainWindow::~MainWindow() {
    for (auto &binding : bindings) delete binding;
    for (auto &viewer : viewers) delete viewer;
    delete ui;
}

void MainWindow::updateUIFromParams() {
    for (auto &binding : bindings) {
        binding->param2UI();
    }
}

void MainWindow::updateParamsFromUI() {
    for (auto &binding : bindings) {
        binding->ui2Params();
    }
}

void MainWindow::loadSelectedDataSet() {
    tuner.loadImageDataSet("/Users/liuzikai/Files/VOCdevkit/VOC");
    ui->imageList->clear();
    for (const auto &image : tuner.getDataSetImages()) {
        ui->imageList->addItem(image.c_str());
    }
    socketClient.connect("127.0.0.1", "8800", MainWindow::handleClientDisconnection);
}

void MainWindow::runSingleDetectionOnImage() {
    updateParamsFromUI();
    tuner.setSharedParams(sharedParams);
    detector.setParams(sharedParams, detectorParams);

    DetectorTuner::RunEvaluation evaluation;
    tuner.runOnSingleImage(ui->imageList->currentItem()->text().toStdString(), armorCenters, evaluation);

    ui->evalResultLabel->setText(
            "Count: " + QString::number(evaluation.imageCount) + "\n" +
            "Time: " + QString::number(evaluation.timeEscapedMS) + " ms"
    );
    setUIFromResults();
}

void MainWindow::switchCamera() {
    if (camera.isOpened()) {
        camera.release();
    } else {
        updateParamsFromUI();
        camera.open(sharedParams, cameraParams);
        ui->cameraInfoLabel->setText(QString::fromStdString(camera.getCapInfo()));
    }
}

void MainWindow::setUIFromResults() const {
    showCVMatInLabel(detector.imgOriginal, QImage::Format_BGR888, ui->originalImage);
    showCVMatInLabel(detector.imgBrightnessThreshold, QImage::Format_Indexed8, ui->brightnessThresholdImage);
    showCVMatInLabel(detector.imgColorThreshold, QImage::Format_Indexed8, ui->colorThresholdImage);
    showCVMatInLabel(detector.noteContours.mat(), QImage::Format_BGR888, ui->contourImage);
    ui->contourCountLabel->setNum(detector.acceptedContourCount);
    showCVMatInLabel(detector.imgArmors, QImage::Format_BGR888, ui->armorImage);
    QString armorResult;
    QTextStream ss(&armorResult);
    ss << "Detected " << armorCenters.size() << " armor";
    if (armorCenters.size() > 1) ss << "s";
    for (const auto &center : armorCenters) {
        ss << "\n(" << center.x << ", " << center.y << ")";
    }
    ss.flush();
    ui->armorResultLabel->setText(armorResult);
}

void MainWindow::updateCameraFrame(void *ptr) {
    auto inst = static_cast<MainWindow *>(ptr);
    showCVMatInLabel(inst->camera.getFrame(), QImage::Format_BGR888, inst->ui->cameraImage);
}

void MainWindow::showCVMatInLabel(const cv::Mat &mat, QImage::Format format, QLabel *label) {
    auto img = QImage(mat.data, mat.cols, mat.rows, format);
    label->setPixmap(QPixmap::fromImage(img).scaledToHeight(label->height(), Qt::SmoothTransformation));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->contourImageWidget) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            viewers.emplace_back(new AnnotatedMatViewer(detector.noteContours, QImage::Format_BGR888));
            viewers.back()->show();
        }
    }
    return QObject::eventFilter(obj, event);
}

}