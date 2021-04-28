#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QtWidgets/QLabel>
#include <QTextStream>
#include <QTimer>
#include <iostream>
#include "Parameters.ui.h"
#include "TerminalParameters.h"

namespace meta {

MainWindow::MainWindow(boost::asio::io_context &ioContext, QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow),
        statsUpdateTimer(new QTimer(this)),
        socket(ioContext, [this](auto c) { handleClientDisconnection(c); }) {

    // Setup UI
    ui->setupUi(this);
    phases = new PhaseController(ui->centralContainer, ui->centralContainerVertialLayout);

    socket.setCallbacks([this](auto name, auto s) { handleRecvSingleString(name, s); },
                        [this](auto name, auto val) { handleRecvSingleInt(name, val); },
                        [this](auto name, auto buf, auto size) { handleRecvBytes(name, buf, size); },
                        [this](auto name, auto list) { handleRecvListOfStrings(name, list); });

    // Setup update timer
    connect(statsUpdateTimer, &QTimer::timeout, this, &MainWindow::updateStats);
    statsUpdateTimer->setSingleShot(false);
    statsUpdateTimer->start(1000);  // update socket stats per second

    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::connectToServer);
    connect(ui->transferImagesCheck, &QCheckBox::stateChanged, [this](auto state) {
        if (state == Qt::Checked) startResultFetchingCycle(); else phases->resetImageLabels();
    });

    connect(ui->stopButton, &QPushButton::clicked, [this] { socket.sendBytes("stop"); });
    connect(ui->startCameraButton, &QPushButton::clicked, [this] { socket.sendBytes("runCamera"); });
//    connect(ui->runSingleImageButton, &QPushButton::clicked, this, &MainWindow::runSingleDetectionOnImage);
//    connect(ui->loadDataSetButton, &QPushButton::clicked, this, &MainWindow::loadSelectedDataSet);

    connect(ui->reloadParamsButton, &QPushButton::clicked, [this] { socket.sendBytes("getParams"); });
    connect(ui->saveParamButton, &QPushButton::clicked, [this] { socket.sendBytes("setParams", phases->getParamSet()); });


}

void MainWindow::connectToServer() {
    if (ui->connectButton->text() == "Connect") {
        if (socket.connect(ui->serverCombo->currentText().toStdString(), TCP_SOCKET_PORT_STR)) {

            ui->statusBar->showMessage( "Connected to " + ui->serverCombo->currentText() + ":" + TCP_SOCKET_PORT_STR);

            // Update UI
            ui->serverCombo->setEnabled(false);
            ui->connectButton->setText("Disconnect");

            // Start fetching cycle
            if (ui->transferImagesCheck->isChecked()) startResultFetchingCycle();

        } else {
            ui->statusBar->showMessage("Failed to connected to " + ui->serverCombo->currentText() + ":" +
                                       TCP_SOCKET_PORT_STR);
        }
    } else {
        socket.disconnect();  // update UI at handleClientDisconnection
    }
}

void MainWindow::handleClientDisconnection(TerminalSocketClient *) {
    ui->statusBar->showMessage(
            "Disconnected from " + ui->serverCombo->currentText() + ":" + TCP_SOCKET_PORT_STR);
    ui->serverCombo->setEnabled(true);
    ui->connectButton->setText("Connect");
}

void MainWindow::handleRecvBytes(std::string_view name, const uint8_t *buf, size_t size) {

    if (name == "res") {

        if (ui->transferImagesCheck->isChecked()) {
            if (!resultMessage.ParseFromArray(buf, size)) {
                ui->statusBar->showMessage("Received an invalid Result package");
            } else {
                phases->applyResults(resultMessage);
            }
            if (resultMessage.has_camera_image()) {
                startResultFetchingCycle();  // continue for next cycle
            }
        }  // Otherwise, discard result and do not send next fetching request

    } else if (name == "params") {

        if (!paramsMessage.ParseFromArray(buf, size)) {
            ui->statusBar->showMessage("Received an invalid ParamSet package");
        } else {
            phases->applyParamSet(paramsMessage);
            ui->statusBar->showMessage("Parameter updated");
        }

    } else {
        ui->statusBar->showMessage("Unknown bytes package <" + QString(name.data()) + ">");
    }
}

void MainWindow::handleRecvSingleString(std::string_view name, std::string_view s) {

    if (name == "msg") {
        ui->statusBar->showMessage(QString(s.data()));
    } else goto INVALID_PACKAGE;

    return;
    INVALID_PACKAGE:
    ui->statusBar->showMessage("Invalid single-string package <" +
                               QString(name.data()) + ">\"" + QString(s.data()) + "\"");
}

void MainWindow::handleRecvSingleInt(std::string_view name, int val) {
    if (name == "fps") {
        ui->fpsLabel->setText(QString::number(val) + " frame/s");
    } else {
        ui->statusBar->showMessage("Unknown int package <" + QString(name.data()) + ">");
    }
}


void MainWindow::handleRecvListOfStrings(std::string_view name, const vector<const char *> &list) {

}

void MainWindow::startResultFetchingCycle() {
    socket.sendBytes("fetch");
}

MainWindow::~MainWindow() {
//    for (auto &viewer : viewers) delete viewer;
    delete phases;
    delete ui;
}

void MainWindow::loadSelectedDataSet() {

    /*ui->imageList->clear();
    for (const auto &image : tuner.getDataSetImages()) {
        ui->imageList->addItem(image.c_str());
    }*/
}

void MainWindow::runSingleDetectionOnImage() {
    /*updateParamsFromUI();
    tuner.setSharedParams(sharedParams);
    detector.setParams(sharedParams, detectorParams);

    DetectorTuner::RunEvaluation evaluation;
    tuner.runOnSingleImage(ui->imageList->currentItem()->text().toStdString(), armorCenters, evaluation);

    ui->evalResultLabel->setText(
            "Count: " + QString::number(evaluation.imageCount) + "\n" +
            "Time: " + QString::number(evaluation.timeEscapedMS) + " ms"
    );
    setUIFromResults();*/
}

void MainWindow::setUIFromResults() const {
    /*showCVMatInLabel(detector.imgOriginal, QImage::Format_BGR888, ui->originalImage);
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
    ui->armorResultLabel->setText(armorResult);*/
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    /*if (obj == ui->contourImageWidget) {
        if (event->type() == QEvent::MouseButtonDblClick) {
//            viewers.emplace_back(new AnnotatedMatViewer(detector.noteContours, QImage::Format_BGR888));
            viewers.back()->show();
        }
    }*/
    return QObject::eventFilter(obj, event);
}

void MainWindow::updateStats() {
    std::pair<unsigned, unsigned> stats = socket.getAndClearStats();  // {sent, received}
    ui->sentBytesLabel->setText(bytesToDateRate(stats.first));
    ui->recvBytesLabel->setText(bytesToDateRate(stats.second));
    socket.sendBytes("fps");  // request for FPS
}

QString MainWindow::bytesToDateRate(unsigned int n) {
    n /= 1024;
    if (n < 1024) {
        return QString::number(n) + " KB/s";
    } else {
        return QString::number(((double) n / 1024.0), 'g', 2) + " MB/s";
    }
}

}