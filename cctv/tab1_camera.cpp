#include "tab1_camera.h"
#include "ui_tab1_camera.h"

#include "motiondetector.h"   // 구현부에서 include
#include <QPixmap>
#include <QDebug>

Tab1_camera::Tab1_camera(QWidget *parent)
    : QWidget(parent), ui(new Ui::Tab1_camera)
{
    ui->setupUi(this);

    // ★ Detector 생성(네가 예전처럼 Tab1이 소유하는 구조로 복구)
    //   camIndex, URL 등은 원래대로 맞춰줘야 함
    m_detector = new MotionDetector(/*camIndex=*/-1, /*parent=*/nullptr);

    // ※ 네 환경에 맞게 카메라 소스 지정
    //    예전에 ws://10.10.16.44:8080 쓰던 흐름이라면 복구:
    m_detector->setCameraUrl(QStringLiteral("http://10.10.16.63:8080/?action=stream"));

    // 필요 옵션: (주석 해제해서 쓰면 됨)
    // m_detector->setAutoClaheEnabled(true);
    // m_detector->setAutoClaheParams(80, 8.0);
    // m_detector->setMog2Params(500, 16.0);

    // ★ Tab1 내부 표시용 연결
    connect(m_detector, &MotionDetector::frameReady,
            this,        qOverload<const QImage&, double>(&Tab1_camera::onFrameReady));
    connect(m_detector, &MotionDetector::detected,
            this,        &Tab1_camera::onDetected);

    // ★ 시작
    m_detector->start();
}

Tab1_camera::~Tab1_camera()
{
    if (m_detector) {
        m_detector->stop();
        delete m_detector;
        m_detector = nullptr;
    }
    delete ui;
}

void Tab1_camera::onFrameReady(const QImage& img, double /*clipLimit*/)
{
    m_lastFrame = img;
    if (m_displayOn && ui->pCam) {
        ui->pCam->setPixmap(QPixmap::fromImage(img).scaled(
            ui->pCam->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void Tab1_camera::onDetected()
{
    qDebug() << "[Tab1] motion detected!";
}

void Tab1_camera::on_pPBCam_clicked()
{
    m_displayOn = !m_displayOn;
    if (!m_displayOn && ui->pCam) {
        ui->pCam->clear();
    } else if (!m_lastFrame.isNull()) {
        onFrameReady(m_lastFrame);
    }
}
