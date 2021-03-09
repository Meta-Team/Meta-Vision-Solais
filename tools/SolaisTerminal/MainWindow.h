#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "AnnotatedMatViewer.h"
#include "ArmorDetector.h"
#include "DetectorTuner.h"
#include "ValueUIBindings.hpp"

namespace Ui {
class MainWindow;
}

class QLabel;

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

    ArmorDetector::ParameterSet params;
    ArmorDetector detector;
    DetectorTuner tuner;


    vector<cv::Point2f> armorCenters;

    static void showCVMatInLabel(const cv::Mat &mat, QImage::Format format, QLabel *label);

    void setUIFromResults() const;

    std::vector<ValueUIBinding *> bindings;

    std::vector<AnnotatedMatViewer *> viewers;

private slots:

    void runSingleDetection();

    void updateUIFromParams();

    void updateParamsFromUI();

    void loadSelectedDataSet();
};

}

#endif // MAINWINDOW_H
