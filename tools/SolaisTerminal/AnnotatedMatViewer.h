#ifndef ANNOTATEDMATVIEWER_H
#define ANNOTATEDMATVIEWER_H

#include <QMainWindow>
#include "AnnotatedMat.h"

namespace Ui {
class AnnotatedMatViewer;
}

namespace meta {

class AnnotatedMatViewer : public QMainWindow {
Q_OBJECT

public:

    AnnotatedMatViewer(const AnnotatedMat &mat, QImage::Format format, QWidget *parent = nullptr);

    ~AnnotatedMatViewer();

protected:

    void mouseMoveEvent(QMouseEvent *event) override;

private:

    Ui::AnnotatedMatViewer *ui;

    const AnnotatedMat &annotatedMat;

};

}

#endif // ANNOTATEDMATVIEWER_H
