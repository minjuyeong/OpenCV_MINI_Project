#include "mainwidget.h"
#include "ui_mainwidget.h"
#include <QVBoxLayout>
#include "streamserver.h" // [추가] StreamServer 헤더 포함

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

    // 1. 스트림 서버 생성 (8080 포트 사용)
    StreamServer *server = new StreamServer(8080, this);

    // 2. Tab1의 MotionDetector가 프레임을 만들 때마다(frameReady 신호)
    //    스트림 서버가 받아서 웹으로 방송하도록(onNewFrame 슬롯) 연결합니다.
    if (auto det = pTab1_camera->detector()) {
        connect(det, &MotionDetector::frameReady, server, &StreamServer::onNewFrame);
    }

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
