#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QtWidgets/QLabel>
#include <QTextStream>
#include <QTimer>
#include <iostream>
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
    ui->setupUi(this);
    phases = new PhaseController(ui->centralContainer, ui->centralContainerVertialLayout);

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

    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::connectToServer);
    connect(ui->transferImagesCheck, &QCheckBox::stateChanged, [this](auto state) {
        if (state == Qt::Checked) socket.sendBytes("fetch"); else phases->resetImageLabels(); });
    connect(ui->reloadListsButton, &QPushButton::clicked, [this] {
        socket.sendBytes("reloadLists"); socket.sendBytes("fetchLists"); });

    connect(ui->stopButton, &QPushButton::clicked, [this] { socket.sendBytes("stop"); });
    connect(ui->runCameraButton, &QPushButton::clicked, [this] {
        socket.sendBytes("runCamera"); socket.sendBytes("fetch"); });
    connect(ui->dataSetList, &QListWidget::currentItemChanged, [this] (auto current, auto previous) {
        if (current) socket.sendSingleString("loadImageDataSet", current->text().toStdString()); });
    connect(ui->imageList, &QListWidget::itemClicked, this, [this] (auto item) {
        if (item) socket.sendSingleString("runImage", item->text().toStdString()); });

    connect(ui->paramSetCombo, &QComboBox::currentTextChanged, [this](const QString &text) {
        socket.sendSingleString("switchParams", text.toStdString());
        socket.sendBytes("getParams");
    });
    connect(ui->reloadParamsButton, &QPushButton::clicked, [this] { socket.sendBytes("getParams"); });
    connect(ui->saveParamButton, &QPushButton::clicked,
            [this] { socket.sendBytes("setParams", phases->getParamSet());});


}

void MainWindow::connectToServer() {
    if (ui->connectButton->text() == "Connect") {
        if (socket.connect(ui->serverCombo->currentText().toStdString(), TCP_SOCKET_PORT_STR)) {

            showStatusMessage("Connected to " + ui->serverCombo->currentText() + ":" + TCP_SOCKET_PORT_STR);

            // Update UI
            ui->serverCombo->setEnabled(false);
            ui->connectButton->setText("Disconnect");

            socket.sendBytes("fetchLists");  // will trigger data set and param set reloads
            if (ui->transferImagesCheck->isChecked()) socket.sendBytes("fetch");  // start fetching cycle

        } else {
            showStatusMessage("Failed to connected to " + ui->serverCombo->currentText() + ":" +
                                       TCP_SOCKET_PORT_STR);
        }
    } else {
        socket.disconnect();  // update UI at handleClientDisconnection
    }
}

void MainWindow::handleClientDisconnection(TerminalSocketClient *) {
    showStatusMessage(
            "Disconnected from " + ui->serverCombo->currentText() + ":" + TCP_SOCKET_PORT_STR);
    ui->serverCombo->setEnabled(true);
    ui->connectButton->setText("Connect");

    phases->resetImageLabels();
    ui->dataSetList->clear();
    ui->imageList->clear();
    ui->paramSetCombo->clear();
}

void MainWindow::handleRecvBytes(std::string_view name, const uint8_t *buf, size_t size) {

    if (name == "res") {

        if (ui->transferImagesCheck->isChecked()) {
            if (!resultMessage.ParseFromArray(buf, size)) {
                showStatusMessage("Received an invalid Result package");
            } else {
                phases->applyResults(resultMessage);
            }
            if (resultMessage.has_camera_image()) {
                socket.sendBytes("fetch");  // continue for next cycle
            }
        }  // Otherwise, discard result and do not send next fetching request

    } else if (name == "params") {

        if (!paramsMessage.ParseFromArray(buf, size)) {
            showStatusMessage("Received an invalid ParamSet package");
        } else {
            phases->applyParamSet(paramsMessage);
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
        ui->paramSetCombo->setCurrentText(QString::fromStdString(string(s)));
        ui->paramSetCombo->blockSignals(false);

    } else goto INVALID_PACKAGE;

    return;
    INVALID_PACKAGE:
    showStatusMessage("Invalid single-string package <" +
                               QString(name.data()) + ">\"" + QString(s.data()) + "\"");
}

void MainWindow::handleRecvSingleInt(std::string_view name, int val) {
    if (name == "fps") {
        ui->fpsLabel->setText(QString::number(val) + " frame/s");
    } else {
        showStatusMessage("Unknown int package <" + QString(name.data()) + ">");
    }
}


void MainWindow::handleRecvListOfStrings(std::string_view name, const vector<const char *> &list) {
    if (name == "imageList") {
        ui->imageList->blockSignals(true);
        ui->imageList->clear();
        for (const auto &image : list) ui->imageList->addItem(image);
        ui->imageList->blockSignals(false);

    } else if (name == "dataSetList") {
        ui->dataSetList->blockSignals(true);
        ui->dataSetList->clear();
        for (const auto &image : list) ui->dataSetList->addItem(image);
        ui->dataSetList->blockSignals(false);
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

MainWindow::~MainWindow() {
//    for (auto &viewer : viewers) delete viewer;
    delete phases;
    delete ui;
}

void MainWindow::showStatusMessage(const QString &text) {
    ui->statusBar->showMessage(text, 5000);
}

void MainWindow::updateStats() {
    std::pair<unsigned, unsigned> stats = socket.getAndClearStats();  // {sent, received}
    ui->sentBytesLabel->setText(bytesToDateRate(stats.first));
    ui->recvBytesLabel->setText(bytesToDateRate(stats.second));
    socket.sendBytes("fps");  // request for FPS
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