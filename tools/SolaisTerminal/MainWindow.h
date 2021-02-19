#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets/QLabel>
#include "ArmorDetector.h"

namespace Ui {
class MainWindow;
}

namespace meta {

class MainWindow : public QMainWindow {
Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

private:
    Ui::MainWindow *ui;

    ArmorDetector::ParameterSet params;
    ArmorDetector detector;

    static void showCVMatInLabel(const cv::Mat &mat, QImage::Format format, QLabel *label);

    void setImages() const;

private slots:

    void runSingleDetection();

    void updateUIFromParams();
};

}

#endif // MAINWINDOW_H
