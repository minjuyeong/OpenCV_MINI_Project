#include "mainwidget.h"
#include "ui_mainwidget.h"
#include <QVBoxLayout>

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);

    // mainwidget.cpp (핵심만)
    pTab1_camera = new Tab1_camera(ui->pTab1);
    auto *lay1 = new QVBoxLayout(ui->pTab1);
    lay1->setContentsMargins(0,0,0,0);
    lay1->setSpacing(0);
    lay1->addWidget(pTab1_camera);
    ui->pTab1->setLayout(lay1);

    pTab2_video = new Tab2_video(ui->pTab2);
    auto *lay2 = new QVBoxLayout(ui->pTab2);
    lay2->setContentsMargins(0,0,0,0);
    lay2->setSpacing(0);
    lay2->addWidget(pTab2_video);
    ui->pTab2->setLayout(lay2);


    // (선택) Tab1 감지시 Tab2에도 팝업 띄우고 싶으면:
    if (auto det = pTab1_camera->detector()) {
        QObject::connect(det, &MotionDetector::detected,
                         pTab2_video, &Tab2_video::onDetected,
                         Qt::QueuedConnection);
    }

    ui->pTabWidget->setCurrentIndex(0);
}

MainWidget::~MainWidget()
{
    delete ui;
}
