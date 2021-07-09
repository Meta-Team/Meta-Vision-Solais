#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets/QListWidget>
//#include "AnnotatedMatViewer.h"
#include "TerminalSocket.h"
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

private:

    Ui::MainWindow *ui;
    PhaseController* phases;
    QTimer *statsUpdateTimer;

    boost::asio::io_context ioContext;
    QTimer *ioTimer;
    TerminalSocketClient socket;

    // Reuse message objects: developers.google.com/protocol-buffers/docs/cpptutorial#optimization-tips
    package::ParamSet paramsMessage;
    package::Result resultMessage;

    int resultPackageCounter = 0;

    void handleClientDisconnection(TerminalSocketClient *client);

    void handleRecvBytes(std::string_view name, const uint8_t *buf, size_t size);

    void handleRecvSingleString(std::string_view name, std::string_view s);

    void handleRecvSingleInt(std::string_view name, int val);

    void handleRecvListOfStrings(std::string_view name, const std::vector<const char *> &list);

    static QString bytesToDateRate(unsigned bytes);

    void showStatusMessage(const QString &text);

    void applyResultMessage();

    void runOnCurrentSelectedImage();

    bool lastRunSingleImage = false;

    /*
     * Solais Core always responses a Result package for a fetch package, but non-empty only if there are meaningful
     * results (the executor is running, for example). If an empty Result package is received, Solais Terminal hold
     * the fetch package until the start of next execution, which is triggered by the executionStarted package.
     */
    bool holdingFetchPackage = false;

    void loadListOfStringsToQListWidget(const std::vector<const char *> &list, QListWidget *listWidget);

    void sendFetch();

private slots:

    void connectToServer();

    void updateStats();

    void performIO();
};

}

#endif // MAINWINDOW_H
