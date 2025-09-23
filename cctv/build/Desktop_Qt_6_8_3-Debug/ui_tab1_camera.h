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
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Tab1_camera
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *pCam;
    QPushButton *pPBCam;

    void setupUi(QWidget *Tab1_camera)
    {
        if (Tab1_camera->objectName().isEmpty())
            Tab1_camera->setObjectName("Tab1_camera");
        Tab1_camera->resize(550, 514);
        verticalLayout = new QVBoxLayout(Tab1_camera);
        verticalLayout->setObjectName("verticalLayout");
        pCam = new QLabel(Tab1_camera);
        pCam->setObjectName("pCam");

        verticalLayout->addWidget(pCam);

        pPBCam = new QPushButton(Tab1_camera);
        pPBCam->setObjectName("pPBCam");

        verticalLayout->addWidget(pPBCam);

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
    } // retranslateUi

};

namespace Ui {
    class Tab1_camera: public Ui_Tab1_camera {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TAB1_CAMERA_H
