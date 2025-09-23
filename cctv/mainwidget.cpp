#include "mainwidget.h"
#include "ui_mainwidget.h"

#include <QVBoxLayout>
#include <QDebug>
#include "streamserver.h"
#include "motiondetector.h"

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);

    // --- Tab1: pTab1 컨테이너에 붙이기(기존 레이아웃 유지) ---
    pTab1_camera = new Tab1_camera(ui->pTab1);
    if (auto lay1 = ui->pTab1->layout()) {
        lay1->addWidget(pTab1_camera);
    } else {
        auto *newLay1 = new QVBoxLayout(ui->pTab1);
        newLay1->setContentsMargins(0,0,0,0);
        newLay1->setSpacing(0);
        newLay1->addWidget(pTab1_camera);
        ui->pTab1->setLayout(newLay1);
    }

    // --- Tab2: pTab2 컨테이너에 붙이기 ---
    pTab2_video = new Tab2_video(ui->pTab2);
    if (auto lay2 = ui->pTab2->layout()) {
        lay2->addWidget(pTab2_video);
    } else {
        auto *newLay2 = new QVBoxLayout(ui->pTab2);
        newLay2->setContentsMargins(0,0,0,0);
        newLay2->setSpacing(0);
        newLay2->addWidget(pTab2_video);
        ui->pTab2->setLayout(newLay2);
    }

    // --- 스트림 서버(웹 방송) ---
    auto *server = new StreamServer(8080, this);   // ← 방송 포트(필요하면 변경)

    if (auto det = pTab1_camera->detector()) {
        // 프레임을 서버로 전달(웹으로 실시간 송출)
        connect(det, &MotionDetector::frameReady,
                server, &StreamServer::onNewFrame);

        // 감지 이벤트를 Tab2에도 전달(선택)
        connect(det, &MotionDetector::detected,
                pTab2_video, &Tab2_video::onDetected,
                Qt::QueuedConnection);
    } else {
        qWarning() << "[MainWidget] MotionDetector is null.";
    }

    if (ui->pTabWidget)
        ui->pTabWidget->setCurrentIndex(0);
}

MainWidget::~MainWidget()
{
    delete ui; // 자식 위젯/서버는 QObject 트리로 함께 정리됨
}
