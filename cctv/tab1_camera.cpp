#include "tab1_camera.h"
#include "ui_tab1_camera.h"
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QVBoxLayout> // ensureCamLabel 폴백을 위해 추가

Tab1_camera::Tab1_camera(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Tab1_camera)
{
    ui->setupUi(this);
    ensureCamLabel();

    connect(ui->pPBCam, &QPushButton::clicked, this, &Tab1_camera::onToggleDisplay);

    m_detector = new MotionDetector(0);
    m_detector->setOutputDirectory(QDir::homePath()+"/Videos/cctv");

    // ✅ 자동 CLAHE 모드를 기본으로 활성화합니다.
    m_detector->setAutoClaheEnabled(m_autoClaheEnabled);
    m_detector->setAutoClaheParams(80, 8.0); // (어둡기 임계값, 최대 필터 강도)

    // ✅ 변경된 시그널/슬롯에 맞게 연결합니다.
    connect(m_detector, &MotionDetector::frameReady, this, &Tab1_camera::onFrameReady);
    connect(m_detector, &MotionDetector::detected, this, &Tab1_camera::onDetected);
    connect(m_detector, &MotionDetector::detectionCleared, this, &Tab1_camera::onDetectionCleared);
    connect(m_detector, &MotionDetector::errorOccured, this, [](const QString& e){ qWarning() << e; });

    m_detector->start();
    setDisplayEnabled(false);
}

Tab1_camera::~Tab1_camera() {
    if (m_detector) m_detector->stop();
    delete ui;
}

// ✅ 스페이스 바를 누르면 자동 CLAHE 모드를 켜고 끄는 로직
void Tab1_camera::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) {
        m_autoClaheEnabled = !m_autoClaheEnabled; // 상태 뒤집기
        QMetaObject::invokeMethod(m_detector, "setAutoClaheEnabled", Q_ARG(bool, m_autoClaheEnabled));
        qDebug() << "[UI] Auto-CLAHE Mode Toggled:" << m_autoClaheEnabled;

        // ✅ UI 라벨 즉시 업데이트 (사용자 경험 향상)
        // UI 파일에 'filterStatusLabel'이라는 이름의 QLabel이 있어야 합니다.
        if (ui->filterStatusLabel) {
            QString offText = m_autoClaheEnabled ? "CLAHE Filter: AUTO" : "CLAHE Filter: OFF";
            ui->filterStatusLabel->setText(offText);
        }

    } else {
        QWidget::keyPressEvent(event);
    }
}

// ✅ [최종] UI를 업데이트하는 슬롯 구현
void Tab1_camera::onFrameReady(const QImage &img, double clipLimit) {
    m_lastFrame = img;
    if (!m_showing) return;

    m_camLabel->setPixmap(QPixmap::fromImage(img));

    // UI 파일에 'filterStatusLabel'이라는 QLabel이 있어야 합니다.
    if (ui->filterStatusLabel) {
        if (clipLimit > 0.0) {
            QString statusText = QString("CLAHE Filter: ON (Strength: %1)")
            .arg(clipLimit, 0, 'f', 1);
            ui->filterStatusLabel->setText(statusText);
        } else {
            QString offText = m_autoClaheEnabled ? "CLAHE Filter: AUTO" : "CLAHE Filter: OFF";
            ui->filterStatusLabel->setText(offText);
        }
    }
}

void Tab1_camera::ensureCamLabel() {
    if (m_camLabel) return;
    m_camLabel = ui->pCam;
    m_camLabel->setAlignment(Qt::AlignCenter);
    m_camLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_camLabel->setScaledContents(true);

    if (!m_badge) {
        m_badge = new QLabel(m_camLabel);
        m_badge->setText(QStringLiteral("감지"));
        m_badge->setStyleSheet("background:#d32f2f;color:white;padding:6px 10px;border-radius:6px;font-weight:600;");
        m_badge->hide();
        m_badge->adjustSize();
    }
}

void Tab1_camera::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    if(m_badge) {
        int x = m_camLabel->width() - m_badge->width() - 10;
        m_badge->move(x, 10);
    }
}

void Tab1_camera::setDisplayEnabled(bool enable) {
    m_showing = enable;
    ui->pPBCam->setText(m_showing ? "카메라 끄기(표시)" : "카메라 켜기(표시)");
    if (!m_showing) m_camLabel->clear();
    else if (!m_lastFrame.isNull()) m_camLabel->setPixmap(QPixmap::fromImage(m_lastFrame));
    else m_camLabel->setText("카메라 준비 중...");
}

void Tab1_camera::onToggleDisplay() { setDisplayEnabled(!m_showing); }

void Tab1_camera::onDetected() {
    if (m_isAlertActive) return;
    m_isAlertActive = true;
    if (m_badge) { m_badge->show(); m_badge->raise(); }
    showPopup();
}

void Tab1_camera::onDetectionCleared() {
    if (!m_isAlertActive) return;
    m_isAlertActive = false;
    if (m_badge) m_badge->hide();
    closePopup();
}

void Tab1_camera::showPopup() {
    if (m_alertBox) return;
    m_alertBox = new QMessageBox(QMessageBox::Warning, "위험 감지", "현관 앞에서 움직임이 감지되었습니다!", QMessageBox::NoButton, this->window());
    m_alertBox->setWindowModality(Qt::NonModal);
    m_alertBox->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    m_alertBox->setAttribute(Qt::WA_DeleteOnClose, true); // 자동 닫힘 시 삭제
    connect(m_alertBox, &QMessageBox::finished, [this](){ m_alertBox = nullptr; });
    m_alertBox->show();
    QTimer::singleShot(3000, m_alertBox, &QMessageBox::accept);
}

void Tab1_camera::closePopup() {
    if (m_alertBox) m_alertBox->close();
}

