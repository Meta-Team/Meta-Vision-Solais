#include "AnnotatedMatViewer.h"
#include "ui_AnnotatedMatViewer.h"
#include <QMouseEvent>

namespace meta {

AnnotatedMatViewer::AnnotatedMatViewer(const meta::AnnotatedMat &annotatedMat, QImage::Format format, QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::AnnotatedMatViewer),
        annotatedMat(annotatedMat) {
    ui->setupUi(this);

    const auto &mat = annotatedMat.mat();

    // Show the Mat in full size
    auto img = QImage(mat.data, mat.cols, mat.rows, format);
    ui->imageLabel->setMinimumSize(img.width(), img.height());
    ui->imageLabel->setMaximumSize(img.width(), img.height());
    ui->imageLabel->setPixmap(QPixmap::fromImage(img));

    this->setWindowTitle(QString::number(mat.cols) + " x " + QString::number(mat.rows));
    ui->statusBar->showMessage("Press and move mouse to view coordinates and annotated RotatedRects");
}

AnnotatedMatViewer::~AnnotatedMatViewer() {
    delete ui;
}

void AnnotatedMatViewer::mouseMoveEvent(QMouseEvent *event) {
    auto pos = event->pos();
    string comment;
    (void) annotatedMat.match(pos.x() - ui->imageLabel->x(),
                              pos.y() - ui->imageLabel->y(),
                              comment);
    ui->statusBar->showMessage("(" + QString::number(pos.x()) + ", " + QString::number(pos.y()) + ") " +
                               " -- " + comment.c_str());
}

}