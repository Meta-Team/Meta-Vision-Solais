//
// Created by liuzikai on 5/26/21.
// Reference: https://stackoverflow.com/questions/32476006/how-to-make-an-expandable-collapsable-section-widget-in-qt
//

#ifndef COLLAPSIBLEGROUPBOX_HPP
#define COLLAPSIBLEGROUPBOX_HPP

#include <QFrame>
#include <QGridLayout>
#include <QParallelAnimationGroup>
#include <QScrollArea>
#include <QToolButton>
#include <QWidget>

class CollapsibleGroupBox : public QWidget {
Q_OBJECT

public:

    explicit CollapsibleGroupBox(QWidget *parent = nullptr) : QWidget(parent) {
        toggleButton = new QToolButton(this);
        toggleButton->setStyleSheet("QToolButton { border: none; }");
        toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toggleButton->setCheckable(true);
        toggleButton->setMaximumHeight(12);
        toggleButton->setMinimumHeight(12);

        // Start expanded
        toggleButton->setArrowType(Qt::ArrowType::DownArrow);
        toggleButton->setChecked(true);

        headerLine = new QFrame(this);
        headerLine->setFrameShape(QFrame::HLine);
        headerLine->setFrameShadow(QFrame::Sunken);
        headerLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        contentArea = new QWidget(this);
        contentArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        // No margin and spacing
        mainLayout = new QGridLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        mainLayout->addWidget(toggleButton,0 , 0, 1, 1, Qt::AlignLeft);
        mainLayout->addWidget(headerLine, 0, 2, 1, 1);
        mainLayout->addWidget(contentArea, 1, 0, 1, 3);

        connect(toggleButton, &QToolButton::clicked, [this](bool checked) {
            toggleButton->setArrowType(checked ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
            contentArea->setMaximumHeight(checked ? 16777215 : 0);
        });
    }

    void setContentLayout(QLayout *contentLayout) { contentArea->setLayout(contentLayout); }

    void setTitle(const QString &title) { toggleButton->setText(title); }

private:

    QGridLayout* mainLayout;
    QToolButton* toggleButton;
    QFrame* headerLine;
    QWidget* contentArea;
};

#endif // COLLAPSIBLEGROUPBOX_H