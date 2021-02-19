#include "MainWindow.h"
#include "ui_MainWindow.h"
#define IMAGE_HEIGHT 200

namespace meta {

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow) {
    ui->setupUi(this);

    updateUIFromParams();

    connect(ui->runButtons, &QPushButton::clicked, this, &MainWindow::runSingleDetection);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::updateUIFromParams() {
    ui->brightnessSpinBox->setValue(params.brightnessThreshold);

    ui->hsvRadio->setChecked(params.colorThresholdMode == ArmorDetector::HSV);
}

void MainWindow::runSingleDetection() {
    params.brightnessThreshold = ui->brightnessSpinBox->value();
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