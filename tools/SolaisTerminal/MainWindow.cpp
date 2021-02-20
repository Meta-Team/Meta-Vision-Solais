#include "MainWindow.h"
#include "ui_MainWindow.h"

namespace meta {

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow) {
    ui->setupUi(this);

    bindings = {
            new ValueCheckSpinBinding<double>(
                    &params.brightnessThreshold,
                    nullptr, ui->brightnessSpin),
            new EnumRadioBinding<ArmorDetector::ColorThresholdMode>(
                    &params.colorThresholdMode,
                    std::vector<std::pair<ArmorDetector::ColorThresholdMode, QRadioButton *>>{
                            {ArmorDetector::HSV,         ui->hsvRadio},
                            {ArmorDetector::RB_CHANNELS, ui->rbChannelRadio},
                    }),
            new RangeCheckSpinBinding<double>(
                    nullptr, &params.hsvRedHue,
                    nullptr, ui->hsvRedHueMinSpin, ui->hsvRedHueMaxSpin),
            new RangeCheckSpinBinding<double>(
                    nullptr, &params.hsvBlueHue,
                    nullptr, ui->hsvBlueHueMinSpin, ui->hsvBlueHueMaxSpin),
            new ValueCheckSpinBinding<double>(
                    &params.rbChannelThreshold,
                    nullptr, ui->rbChannelThresholdSpin),
            new ValueCheckSpinBinding<int>(
                    &params.colorDilate,
                    ui->colorDliateCheck, ui->colorDliateSpin),
            new ValueCheckSpinBinding<double>(
                    &params.contourRectMinSize,
                    ui->rectMinSizeCheck, ui->rectMinSizeSpin),
            new ValueCheckSpinBinding<double>(
                    &params.contourMinArea,
                    ui->contourMinAreaCheck, ui->contourMinAreaSpin),
            new ValueCheckSpinBinding<int>(
                    &params.longEdgeMinLength,
                    ui->longEdgeMinLengthCheck, ui->longEdgeMinLengthSpin),
            new RangeCheckSpinBinding<double>(&params.enableAspectRatioFilter, &params.aspectRatio,
                    ui->aspectRatioFilterCheck, ui->aspectRatioMinSpin, ui->aspectRatioMaxSpin),

    };

    updateUIFromParams();
    connect(ui->runButtons, &QPushButton::clicked, this, &MainWindow::runSingleDetection);
}

MainWindow::~MainWindow() {
    for (auto &binding : bindings) delete binding;
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

void MainWindow::runSingleDetection() {
    updateParamsFromUI();
    detector.setParams(params);
    detector.detect(cv::imread("/Users/liuzikai/Files/VOCdevkit/VOC/JPEGImages/305.jpg"));
    setImages();
}

void MainWindow::setImages() const {
    showCVMatInLabel(detector.imgOriginal, QImage::Format_BGR888, ui->originalImage);
    showCVMatInLabel(detector.imgGray, QImage::Format_Grayscale8, ui->grayImage);
    showCVMatInLabel(detector.imgBrightnessThreshold, QImage::Format_Indexed8, ui->brightnessThresholdImage);
    showCVMatInLabel(detector.imgColorThreshold, QImage::Format_Indexed8, ui->colorThresholdImage);
    showCVMatInLabel(detector.imgContours, QImage::Format_BGR888, ui->contourImage);
}

void MainWindow::showCVMatInLabel(const cv::Mat &mat, QImage::Format format, QLabel *label) {
    // label->setMinimumSize(0, IMAGE_HEIGHT);
    auto img = QImage(mat.data, mat.cols, mat.rows, format);
    label->setPixmap(QPixmap::fromImage(img).scaledToHeight(label->height(), Qt::SmoothTransformation));
}

}