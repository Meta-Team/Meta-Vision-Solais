#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QtWidgets/QLabel>
#include <QTextStream>
#include <QTimer>
#include <iostream>
#include <QPainter>
#include "Parameters.ui.h"
#include "TerminalParameters.h"

namespace meta {

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow),
        ioTimer(new QTimer(this)),
        statsUpdateTimer(new QTimer(this)),
        socket(ioContext, [this](auto c) { handleClientDisconnection(c); }) {

    // Setup UI
    setWindowTitle("Meta-Vision-Solais Terminal");
    ui->setupUi(this);
    phases = new PhaseController(ui->centralContainer, ui->centralContainerVertialLayout, this);

    // Setup IO
    socket.setCallbacks([this](auto name, auto s) { handleRecvSingleString(name, s); },
                        [this](auto name, auto val) { handleRecvSingleInt(name, val); },
                        [this](auto name, auto buf, auto size) { handleRecvBytes(name, buf, size); },
                        [this](auto name, auto list) { handleRecvListOfStrings(name, list); });
    // Socket callbacks are called from the thread of io_context. Qt doesn't allow operating GUI from another thread.
    // So timer is used to handle the IO operations from the current thread.
    connect(ioTimer, &QTimer::timeout, this, &MainWindow::performIO);
    ioTimer->setSingleShot(false);
    ioTimer->start(20);

    // Setup update timer
    connect(statsUpdateTimer, &QTimer::timeout, this, &MainWindow::updateStats);
    statsUpdateTimer->setSingleShot(false);
    statsUpdateTimer->start(1000);  // update socket stats per second

    // Setup signals and functions

    // Phases
    connect(phases, &PhaseController::parameterEdited, [this] {
        ui->reloadParamsButton->setStyleSheet("font-weight: bold; color: red");
        ui->saveParamButton->setStyleSheet("font-weight: bold; color: red");
    });

    // Connection
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::connectToServer);
    connect(ui->transferImagesCheck, &QCheckBox::stateChanged, [this](auto state) {
        if (state == Qt::Checked) {
            sendFetch();
        } else {
            phases->resetImageLabels();
        }
        holdingFetchPackage = false;  // the on-hold fetch package is either sent or discarded
    });
    connect(ui->reloadListsButton, &QPushButton::clicked, [this] {
        socket.sendBytes("reloadLists");
        socket.sendBytes("fetchLists");
    });

    // Execution
    connect(ui->stopButton, &QPushButton::clicked, [this] {
        socket.sendBytes("stop");
    });
    connect(ui->runCameraButton, &QPushButton::clicked, [this] {
        socket.sendBytes("runCamera");
        lastRunSingleImage = false;
    });
    connect(ui->captureImageButton, &QPushButton::clicked, [this] {
        socket.sendBytes("captureImage");
    });
    connect(ui->startRecordButton, &QPushButton::clicked, [this] {
        socket.sendBytes("startRecord");
    });
    connect(ui->stopRecordButton, &QPushButton::clicked, [this] {
        socket.sendBytes("stopRecord");
    });
    connect(ui->imageSetList, &QListWidget::currentItemChanged, [this](auto current, auto previous) {
        if (current) socket.sendSingleString("switchImageSet", current->text().toStdString());
        lastRunSingleImage = false;
    });
    connect(ui->imageList, &QListWidget::currentItemChanged, this, &MainWindow::runOnCurrentSelectedImage);
    connect(ui->videoList, &QListWidget::currentItemChanged, [this](auto current, auto previous) {
        if (current) socket.sendSingleString("previewVideo", current->text().toStdString());
    });
    connect(ui->runImageSetButton, &QPushButton::clicked, [this] {
        socket.sendBytes("runImageSet");
        lastRunSingleImage = false;
    });
    connect(ui->runVideoButton, &QPushButton::clicked, [this] {
        socket.sendSingleString("runVideo", ui->videoList->currentItem()->text().toStdString());
    });

    // Parameters
    connect(ui->paramSetCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        socket.sendSingleString("switchParamSet", text.toStdString());
        socket.sendBytes("getParams");
    });
    connect(ui->reloadParamsButton, &QPushButton::clicked, [this] {
        socket.sendBytes("getParams");
    });
    connect(ui->saveParamButton, &QPushButton::clicked, [this] {
        socket.sendBytes("setParams", phases->getParamSet());
        ui->reloadParamsButton->setStyleSheet("");
        ui->saveParamButton->setStyleSheet("");
        if (lastRunSingleImage) runOnCurrentSelectedImage();
    });


}

void MainWindow::connectToServer() {
    if (ui->connectButton->text() == "Connect") {
        if (socket.connect(ui->serverCombo->currentText().toStdString(), TCP_SOCKET_PORT_STR)) {

            showStatusMessage("Connected to " + ui->serverCombo->currentText() + ":" + TCP_SOCKET_PORT_STR);

            if (ui->transferImagesCheck->isChecked()) {
                holdingFetchPackage = true;
            }

            // Update UI
            ui->serverCombo->setEnabled(false);
            ui->connectButton->setText("Disconnect");

            socket.sendBytes("fetchLists");  // will trigger data set and param set reloads

        } else {
            showStatusMessage("Failed to connected to " + ui->serverCombo->currentText() + ":" +
                              TCP_SOCKET_PORT_STR);
        }
    } else {
        socket.disconnect();  // update UI at handleClientDisconnection
    }
}

void MainWindow::sendFetch() {
    if (ui->fetchImageCheck->isChecked()) {
        socket.sendSingleString("fetch",
                                (std::string) (phases->cameraImageLabel->visibleRegion().isEmpty() &&
                                               phases->armorImageLabel->visibleRegion().isEmpty() ? "F" : "T") +
                                (phases->brightnessImageLabel->visibleRegion().isEmpty() ? "F" : "T") +
                                (phases->colorImageLabel->visibleRegion().isEmpty() ? "F" : "T") +
                                (phases->contourImageLabel->visibleRegion().isEmpty() ? "F" : "T"));
    } else {
        socket.sendSingleString("fetch", "FFFF");
    }
}

void MainWindow::handleClientDisconnection(TerminalSocketClient *) {
    showStatusMessage(
            "Disconnected from " + ui->serverCombo->currentText() + ":" + TCP_SOCKET_PORT_STR);
    ui->serverCombo->setEnabled(true);
    ui->connectButton->setText("Connect");

    phases->resetImageLabels();
    ui->imageSetList->clear();
    ui->imageList->clear();
    ui->paramSetCombo->clear();
}

void MainWindow::handleRecvBytes(std::string_view name, const uint8_t *buf, size_t size) {

    if (name == "res") {

        if (ui->transferImagesCheck->isChecked()) {
            if (size == 0) {
                // Do not send another fetch, but hold the fetch package
                holdingFetchPackage = true;

                showStatusMessage("Execution stopped");
            } else {
                if (!resultMessage.ParseFromArray(buf, size)) {
                    showStatusMessage("Received an invalid Result package");
                } else {
                    ++resultPackageCounter;
                    applyResultMessage();
                }
                sendFetch();  // continue for next cycle
            }
        }  // Otherwise, discard result and do not send next fetching request

    } else if (name == "params") {

        if (!paramsMessage.ParseFromArray(buf, size)) {
            showStatusMessage("Received an invalid ParamSet package");
        } else {
            phases->applyParamSet(paramsMessage);
            ui->reloadParamsButton->setStyleSheet("");
            ui->saveParamButton->setStyleSheet("");
        }

    } else {
        showStatusMessage("Unknown bytes package <" + QString(name.data()) + ">");
    }
}

void MainWindow::handleRecvSingleString(std::string_view name, std::string_view s) {

    if (name == "msg") {
        showStatusMessage(QString(s.data()));

    } else if (name == "currentParamSetName") {
        ui->paramSetCombo->blockSignals(true);
        ui->paramSetCombo->setCurrentText(QString::fromStdString(std::string(s)));
        ui->paramSetCombo->blockSignals(false);

    } else if (name == "executionStarted") {
        showStatusMessage("Start execution on " + QString::fromStdString(std::string(s)));
        if (holdingFetchPackage || lastRunSingleImage) {
            sendFetch();
            holdingFetchPackage = false;
        }

    } else goto INVALID_PACKAGE;

    return;
    INVALID_PACKAGE:
    showStatusMessage("Invalid single-string package <" +
                      QString(name.data()) + ">\"" + QString(s.data()) + "\"");
}

void MainWindow::handleRecvSingleInt(std::string_view name, int val) {
    {
        showStatusMessage("Unknown int package <" + QString(name.data()) + ">");
    }
}


void MainWindow::handleRecvListOfStrings(std::string_view name, const std::vector<const char *> &list) {
    if (name == "fps") {
        if (list.size() == 3) {
            ui->inputFPSLabel->setText(QString(list[0]) + " frames/s");
            ui->executionFPSLabel->setText(QString(list[1]) + " frames/s");
            ui->serialFPSLabel->setText(QString(list[2]) + " pkgs/s");
        } else {
            showStatusMessage("Invalid fps package size " + QString::number(list.size()));
        }


    } else if (name == "imageList") {
        loadListOfStringsToQListWidget(list, ui->imageList);

    } else if (name == "videoList") {
        loadListOfStringsToQListWidget(list, ui->videoList);

    } else if (name == "imageSetList") {
        loadListOfStringsToQListWidget(list, ui->imageSetList);
        ui->imageList->clear();  // current data set is reset

    } else if (name == "paramSetList") {
        ui->paramSetCombo->blockSignals(true);
        ui->paramSetCombo->clear();
        for (const auto &paramSet : list) ui->paramSetCombo->addItem(paramSet);
        ui->paramSetCombo->blockSignals(false);

        // Reload
        socket.sendBytes("getCurrentParamSetName");
        socket.sendBytes("getParams");

    } else {
        showStatusMessage("Unknown list-of-string package <" + QString(name.data()) + ">");
    }
}

void MainWindow::loadListOfStringsToQListWidget(const std::vector<const char *> &list, QListWidget *listWidget) {
    listWidget->blockSignals(true);
    listWidget->clear();
    for (const auto &image : list) listWidget->addItem(image);
    listWidget->blockSignals(false);
}

void MainWindow::runOnCurrentSelectedImage() {
    if (ui->imageList->currentItem()) {
        socket.sendSingleString("runImage", ui->imageList->currentItem()->text().toStdString());
        lastRunSingleImage = true;
    }
}

MainWindow::~MainWindow() {
    delete phases;
    delete ui;
}

void MainWindow::showStatusMessage(const QString &text) {
    ui->statusBar->showMessage(text, 5000);
}

void MainWindow::applyResultMessage() {

    // GROUP: Input
    if (resultMessage.has_camera_info()) {
        phases->cameraInfoLabel->setText(QString::fromStdString(resultMessage.camera_info()));
    }
    if (resultMessage.has_camera_image()) {
        if (!resultMessage.camera_image().data().empty()) {
            phases->cameraImage = QImage::fromData((const uint8_t *) resultMessage.camera_image().data().c_str(),
                                                   resultMessage.camera_image().data().size()).copy();
            phases->cameraImageLabel->setPixmap(QPixmap::fromImage(phases->cameraImage));
        } else {
            phases->cameraImageLabel->setText("Empty");
        }
    }

    // GROUP: Brightness
    if (resultMessage.has_brightness_image()) {
        if (!resultMessage.brightness_image().data().empty()) {
            phases->brightnessImage = QImage::fromData(
                    (const uint8_t *) resultMessage.brightness_image().data().c_str(),
                    resultMessage.brightness_image().data().size()).copy();
            phases->brightnessImageLabel->setPixmap(QPixmap::fromImage(phases->brightnessImage));
        } else {
            phases->brightnessImageLabel->setText("Empty");
        }
    }

    // GROUP: Color
    if (resultMessage.has_color_image()) {
        if (!resultMessage.color_image().data().empty()) {
            phases->colorImage = QImage::fromData((const uint8_t *) resultMessage.color_image().data().c_str(),
                                                  resultMessage.color_image().data().size()).copy();
            phases->colorImageLabel->setPixmap(QPixmap::fromImage(phases->colorImage));
        } else {
            phases->colorImageLabel->setText("Empty");
        }
    }

    // GROUP: Contours
    if (resultMessage.has_contour_image()) {
        if (!resultMessage.contour_image().data().empty()) {
            phases->contourImage = QImage::fromData((const uint8_t *) resultMessage.contour_image().data().c_str(),
                                                    resultMessage.contour_image().data().size())
                    .copy()
                    .convertToFormat(QImage::Format_BGR888);
            QPainter painter(&phases->contourImage);
            painter.setPen(Qt::yellow);
            for (const auto &rect : resultMessage.lights()) {
                painter.resetTransform();
                painter.translate(rect.center().x(), rect.center().y());
                painter.rotate(rect.angle());
                painter.drawRect(-rect.size().x() / 2, -rect.size().y() / 2, rect.size().x(), rect.size().y());
            }
            phases->contourImageLabel->setPixmap(QPixmap::fromImage(phases->contourImage));
        } else {
            phases->contourImageLabel->setText("Empty");
        }
    }

    // GROUP: TopKiller
    {
        QString s;
        QTextStream ss(&s);
        for (const auto &pulse : resultMessage.tk_pulses()) {
            ss << "[" << pulse.time() << "] ("
               << QString::number(pulse.mid_ypd().x(), 'f', 1) << ", "
               << QString::number(pulse.mid_ypd().y(), 'f', 1) << ", "
               << QString::number(pulse.mid_ypd().z(), 'f', 1) << ")\n";
        }
        ss << (resultMessage.tk_triggered() ? "Triggered!" : "Not triggered") << "\n";
        phases->topKillerInfoLabel->setText(s);
    }

    // GROUP: Armors
    if (resultMessage.armors_size() != 0) {

        phases->armorImage = phases->cameraImage.copy();
        QPainter painter(&phases->armorImage);

        QString s;
        QTextStream ss(&s);
        for (const auto &armorInfo : resultMessage.armors()) {

            // Image points
            if (armorInfo.image_points_size() != 4) {
                showStatusMessage("Invalid armor points");
                continue;
            }
            painter.setPen(armorInfo.selected() ? Qt::red : Qt::yellow);
            const auto &points = armorInfo.image_points();
            for (int i = 0; i < 4; i++) {
                painter.drawLine(points[i].x(), points[i].y(), points[(i + 1) % 4].x(), points[(i + 1) % 4].y());
            }

            // Image center
            painter.drawPoint(armorInfo.image_center().x(), armorInfo.image_center().y());

            // Large/small armor, number, and offset
            if (armorInfo.large_armor()) {
                ss << "{" << armorInfo.number() << "} ";
            } else {
                ss << "[" << armorInfo.number() << "] ";
            }
            ss << QString::number(armorInfo.ypd().x(), 'f', 1) << ", "
               << QString::number(armorInfo.ypd().y(), 'f', 1) << ", "
               << QString::number(armorInfo.ypd().z(), 'f', 0) << "\n";
        }

        phases->armorInfoLabel->setText(s);
        phases->armorImageLabel->setPixmap(QPixmap::fromImage(phases->armorImage));
    } else {
        phases->armorInfoLabel->setText("");
        phases->armorImageLabel->setText("Empty");
    }

    // GROUP: Aiming
    {
        QString s;
        QTextStream ss(&s);
        if (resultMessage.has_aiming_target()) {
            ss << QString::number(resultMessage.aiming_target().x(), 'f', 2) << ", "
               << QString::number(resultMessage.aiming_target().y(), 'f', 2) << " | "
               << resultMessage.remaining_time_to_target();
        }
        phases->aimingInfoLabel->setText(s);
    }

}

void MainWindow::updateStats() {
    std::pair<unsigned, unsigned> stats = socket.getAndClearStats();  // {sent, received}
    ui->sentBytesLabel->setText(bytesToDateRate(stats.first));
    ui->recvBytesLabel->setText(bytesToDateRate(stats.second));
    ui->resultFPSLabel->setText(QString::number(resultPackageCounter) + " pkgs/s");
    resultPackageCounter = 0;
    socket.sendBytes("fps");  // request for FPS, handled by callback
}

void MainWindow::performIO() {
    ioContext.poll();
    if (ioContext.stopped()) ioContext.restart();
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