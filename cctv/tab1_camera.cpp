#include "tab1_camera.h"
#include "ui_tab1_camera.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QDir>
#include <QDebug>
#include <QMetaType>

Tab1_camera::Tab1_camera(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Tab1_camera)
{
    ui->setupUi(this);
    ensureCamLabel();

    // --- 버튼 연결 ---
    QPushButton* btn = ui->pPBCam ? ui->pPBCam : this->findChild<QPushButton*>("pPBCam");
    if (btn) {
        connect(btn, &QPushButton::clicked, this, &Tab1_camera::onToggleDisplay);
    } else {
        qWarning() << "[Tab1] pPBCam 버튼을 찾을 수 없습니다. (objectName=pPBCam)";
    }

    // --- MotionDetector 준비 ---
    m_detector = new MotionDetector(0); // parent 주지 않음 (moveToThread 가능)
    m_detector->setOutputDirectory(QDir::homePath()+"/Videos/cctv");
    m_detector->setRecordingSeconds(8);

    // ★ start()보다 '먼저' 신호 연결을 건다
    connect(m_detector, &MotionDetector::frameReady,
            this, &Tab1_camera::onFrameReady, Qt::QueuedConnection);
    connect(m_detector, &MotionDetector::detected,
            this, &Tab1_camera::onDetected,   Qt::QueuedConnection);
    connect(m_detector, &MotionDetector::errorOccured,
            this, [](const QString& e){ qWarning() << e; });
    // [추가] 위험 해제 신호와 슬롯 연결
    connect(m_detector, &MotionDetector::detectionCleared, this, &Tab1_camera::onDetectionCleared, Qt::QueuedConnection);

    // 이제 시작
    m_detector->start();

    // 처음엔 표시 꺼둠
    setDisplayEnabled(false);
}

Tab1_camera::~Tab1_camera()
{
    if (m_detector) m_detector->stop();
    delete ui;
}

// === pCam(QLabel)을 '그대로' 화면 라벨로 사용 ===
void Tab1_camera::ensureCamLabel()
{
    if (m_camLabel) return;

    // ui의 pCam을 그대로 사용 (새 QLabel 생성 X)
    m_camLabel = ui->pCam ? qobject_cast<QLabel*>(ui->pCam)
                          : this->findChild<QLabel*>("pCam");

    if (!m_camLabel) {
        // 예외 상황 폴백
        m_camLabel = new QLabel(this);
        auto *lay = qobject_cast<QVBoxLayout*>(this->layout());
        if (!lay) {
            lay = new QVBoxLayout(this);
            lay->setContentsMargins(0,0,0,0);
            lay->setSpacing(0);
        }
        lay->insertWidget(0, m_camLabel, 1);
    }

    m_camLabel->setAlignment(Qt::AlignCenter);
    m_camLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_camLabel->setMinimumSize(100, 60);
    m_camLabel->setScaledContents(true);

    if (!m_badge) {
        QWidget* host = m_camLabel->parentWidget() ? m_camLabel->parentWidget() : this;
        m_badge = new QLabel(host);
        m_badge->setText(QStringLiteral("감지"));
        m_badge->setStyleSheet("background:#d32f2f;color:white;padding:6px 10px;border-radius:6px;font-weight:600;");
        m_badge->hide();
        m_badge->raise();
    }
}


void Tab1_camera::setDisplayEnabled(bool enable)
{
    m_showing = enable;

    if (QPushButton* btn = ui->pPBCam ? ui->pPBCam : this->findChild<QPushButton*>("pPBCam")) {
        btn->setText(m_showing ? QStringLiteral("카메라 끄기(표시)")
                               : QStringLiteral("카메라 켜기(표시)"));
    }

    if (!m_showing) {
        if (m_camLabel) m_camLabel->clear();
        return;
    }

    // 표시 켰을 때 마지막 프레임이 있으면 즉시 뿌려주기
    if (!m_lastFrame.isNull() && m_camLabel) {
        m_camLabel->setPixmap(QPixmap::fromImage(m_lastFrame));
    } else {
        // 아직 프레임이 안 왔다면 안내 텍스트 — 첫 프레임 오면 onFrameReady가 덮어씀
        if (m_camLabel) m_camLabel->setText(QStringLiteral("카메라 준비 중..."));
    }
}

void Tab1_camera::onToggleDisplay()
{
    const bool turningOn = !m_showing;
    qDebug() << "[Tab1] Toggle display clicked. showing ->" << turningOn;
    setDisplayEnabled(turningOn);
}

void Tab1_camera::onFrameReady(const QImage &img)
{
    m_lastFrame = img;

    if (!m_showing || !m_camLabel) return;

    // setScaledContents(true) 덕분에 별도 스케일링 불필요
    m_camLabel->setPixmap(QPixmap::fromImage(img));

    // (필요 시) 현재 라벨 사이즈 확인
    // qDebug() << "[Tab1] pCam size =" << m_camLabel->size();
}

void Tab1_camera::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);

    if (m_badge && m_badge->parentWidget()) {
        QWidget* camPanel = qobject_cast<QWidget*>(m_badge->parent());
        int x = camPanel->width()  - m_badge->sizeHint().width() - 10;
        int y = 10;
        m_badge->move(x, y);
    }

    if (m_showing && !m_lastFrame.isNull() && m_camLabel) {
        m_camLabel->setPixmap(QPixmap::fromImage(m_lastFrame));
    }
}

void Tab1_camera::showPopup()
{
    // if (m_alertBox && m_alertBox->isVisible()) return;

    // QWidget *top = this->window();
    // m_alertBox = new QMessageBox(QMessageBox::Information,
    //                              QStringLiteral("알림"),
    //                              QStringLiteral("감지"),
    //                              QMessageBox::Ok,
    //                              top);
    // m_alertBox->setWindowModality(Qt::ApplicationModal);
    // m_alertBox->setWindowFlag(Qt::Dialog, true);
    // m_alertBox->setWindowFlag(Qt::CustomizeWindowHint, true);
    // m_alertBox->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    // m_alertBox->setAttribute(Qt::WA_DeleteOnClose, true);
    if (m_alertBox) return;

    QWidget *top = this->window();
    m_alertBox = new QMessageBox(QMessageBox::Warning,
                                 QStringLiteral("위험 감지"),
                                 QStringLiteral("현관 앞에서 움직임이 감지되었습니다!"),
                                 QMessageBox::NoButton, top);
    m_alertBox->setWindowModality(Qt::NonModal);
    m_alertBox->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    m_alertBox->setAttribute(Qt::WA_DeleteOnClose, false); // 자동 삭제 비활성화


    m_alertBox->show();
    m_alertBox->raise();
    m_alertBox->activateWindow();

    QScreen *scr = top ? top->screen() : QGuiApplication::primaryScreen();
    const QRect sr = scr ? scr->availableGeometry()
                         : QGuiApplication::primaryScreen()->availableGeometry();
    const QSize sz = m_alertBox->sizeHint();
    m_alertBox->move(sr.center() - QPoint(sz.width()/2, sz.height()/2));

    QTimer *autoClose = new QTimer(m_alertBox);
    autoClose->setSingleShot(true);
    QObject::connect(autoClose, &QTimer::timeout, m_alertBox, &QMessageBox::accept);
    autoClose->start(3000);
}

void Tab1_camera::onDetected()
{
    // 이미 경보 상태이면 아무것도 하지 않음 (중복 방지)
    if (m_isAlertActive) return;

    m_isAlertActive = true; // 경보 상태로 변경
    if (m_badge) { m_badge->show(); m_badge->raise(); }
    showPopup();
}
// [추가] 팝업창을 내리는 로직
void Tab1_camera::onDetectionCleared()
{
    if (!m_isAlertActive) return; // 경보 상태가 아니면 아무것도 안 함

    m_isAlertActive = false; // 경보 상태 해제
    if (m_badge) { m_badge->hide(); }
    closePopup();
}
// [추가] 팝업을 닫는 함수
void Tab1_camera::closePopup()
{
    if (m_alertBox) {
        m_alertBox->close();
        m_alertBox->deleteLater();
    }
}
