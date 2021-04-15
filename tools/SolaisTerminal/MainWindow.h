#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "AnnotatedMatViewer.h"
#include "ArmorDetector.h"
#include "DetectorTuner.h"
#include "Camera.h"
#include "TerminalSocket.h"
#include "ValueUIBindings.hpp"

namespace Ui {
class MainWindow;
}

class QLabel;
class QTimer;

namespace meta {

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

protected:

    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::MainWindow *ui;
    QTimer *statsUpdateTimer;

    boost::asio::io_context ioContext;
    std::thread ioThread;

    SharedParameters sharedParams;

    ArmorDetector::ParameterSet detectorParams;
    ArmorDetector detector;

    DetectorTuner tuner;

    Camera::ParameterSet cameraParams;
    Camera camera;

    vector<cv::Point2f> armorCenters;

    static void showCVMatInLabel(const cv::Mat &mat, QImage::Format format, QLabel *label);

    void setUIFromResults() const;

    static void updateCameraFrame(void *ptr);

    std::vector<ValueUIBinding *> bindings;

    std::vector<AnnotatedMatViewer *> viewers;

    TerminalSocketClient socket;

    void handleClientDisconnection(TerminalSocketClient *client);

    void handleRecvBytes(std::string_view name, const uint8_t *buf, size_t size);

    void handleRecvSingleString(std::string_view name, std::string_view s);

    void handleRecvListOfStrings(std::string_view name, const vector<const char *> &list);

    static QString bytesToDateRate(unsigned bytes);

private slots:

    void runSingleDetectionOnImage();

    void updateUIFromParams();

    void updateParamsFromUI();

    void loadSelectedDataSet();

    void connectToServer();

    void updateSocketStats();
};

}

#endif // MAINWINDOW_H
