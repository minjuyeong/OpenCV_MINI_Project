#ifndef TAB1_CAMERA_H
#define TAB1_CAMERA_H

#include <QWidget>
#include <QLabel>
#include <QMessageBox>
#include <QPointer>
#include <QImage>

class QPushButton;
class QVBoxLayout;

#include "motiondetector.h"

namespace Ui { class Tab1_camera; }

class Tab1_camera : public QWidget
{
    Q_OBJECT
public:
    explicit Tab1_camera(QWidget *parent = nullptr);
    ~Tab1_camera();

    MotionDetector* detector() const { return m_detector; }

private slots:
    void onToggleDisplay();                // 버튼 클릭 → 화면 표시 on/off
    void onFrameReady(const QImage& img);  // 카메라 프레임 수신
    void onDetected();                     // 감지 팝업

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    void ensureCamLabel();                 // pCam 안에 라벨 세팅
    void setDisplayEnabled(bool enable);   // 표시 on/off 토글
    void showPopup();                      // 팝업(3초 자동 닫힘)

private:
    Ui::Tab1_camera *ui = nullptr;

    QLabel *m_camLabel = nullptr;          // 영상 표시 라벨
    QLabel *m_badge    = nullptr;          // 우상단 "감지" 배지(보조)
    bool    m_showing  = false;            // 표시 on/off 상태

    QPointer<QMessageBox> m_alertBox;      // 팝업 핸들
    MotionDetector *m_detector = nullptr;  // 백그라운드 감지/녹화
    QImage m_lastFrame;                    // 마지막 프레임(즉시 표시용)
};

#endif // TAB1_CAMERA_H
