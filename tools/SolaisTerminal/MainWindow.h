#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "AnnotatedMatViewer.h"
#include "TerminalSocket.h"
#include "ValueUIBindings.hpp"
#include "Parameters.pb.h"

namespace Ui {
class MainWindow;
}

class QLabel;
class QTimer;

namespace meta {

class PhaseController;

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

protected:

    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::MainWindow *ui;
    PhaseController* phases;
    QTimer *statsUpdateTimer;

    boost::asio::io_context ioContext;
    std::thread ioThread;

    static void showCVMatInLabel(const cv::Mat &mat, QImage::Format format, QLabel *label);

    void setUIFromResults() const;

    static void updateCameraFrame(void *ptr);

    std::vector<ValueUIBinding *> bindings;

    std::vector<AnnotatedMatViewer *> viewers;

    TerminalSocketClient socket;

    // Reuse message objects: developers.google.com/protocol-buffers/docs/cpptutorial#optimization-tips
    package::Result resultMessage;

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
