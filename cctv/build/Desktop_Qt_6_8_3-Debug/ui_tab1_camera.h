/********************************************************************************
** Form generated from reading UI file 'tab1_camera.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TAB1_CAMERA_H
#define UI_TAB1_CAMERA_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Tab1_camera
{
public:
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QLabel *pCam;
    QHBoxLayout *horizontalLayout;
    QPushButton *pPBCam;
    QLabel *filterStatusLabel;

    void setupUi(QWidget *Tab1_camera)
    {
        if (Tab1_camera->objectName().isEmpty())
            Tab1_camera->setObjectName("Tab1_camera");
        Tab1_camera->resize(550, 514);
        Tab1_camera->setMinimumSize(QSize(0, 514));
        verticalLayoutWidget = new QWidget(Tab1_camera);
        verticalLayoutWidget->setObjectName("verticalLayoutWidget");
        verticalLayoutWidget->setGeometry(QRect(10, 10, 531, 491));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setSizeConstraint(QLayout::SizeConstraint::SetDefaultConstraint);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        pCam = new QLabel(verticalLayoutWidget);
        pCam->setObjectName("pCam");

        verticalLayout->addWidget(pCam);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        pPBCam = new QPushButton(verticalLayoutWidget);
        pPBCam->setObjectName("pPBCam");

        horizontalLayout->addWidget(pPBCam);

        filterStatusLabel = new QLabel(verticalLayoutWidget);
        filterStatusLabel->setObjectName("filterStatusLabel");

        horizontalLayout->addWidget(filterStatusLabel);

        horizontalLayout->setStretch(0, 8);
        horizontalLayout->setStretch(1, 2);

        verticalLayout->addLayout(horizontalLayout);

        verticalLayout->setStretch(0, 9);
        verticalLayout->setStretch(1, 1);

        retranslateUi(Tab1_camera);

        QMetaObject::connectSlotsByName(Tab1_camera);
    } // setupUi

    void retranslateUi(QWidget *Tab1_camera)
    {
        Tab1_camera->setWindowTitle(QCoreApplication::translate("Tab1_camera", "Form", nullptr));
        pCam->setText(QString());
        pPBCam->setText(QCoreApplication::translate("Tab1_camera", " \354\271\264\353\251\224\353\235\274 \354\274\234\352\270\260", nullptr));
        filterStatusLabel->setText(QCoreApplication::translate("Tab1_camera", "\355\225\204\355\204\260 \352\260\225\353\217\204", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Tab1_camera: public Ui_Tab1_camera {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TAB1_CAMERA_H
