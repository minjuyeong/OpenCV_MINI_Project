#ifndef TAB1_CAMERA_H
#define TAB1_CAMERA_H

#include <QWidget>
#include <QImage>

class MotionDetector;  // 전방 선언 (헤더 경량화)

namespace Ui { class Tab1_camera; }

class Tab1_camera : public QWidget
{
    Q_OBJECT
public:
    explicit Tab1_camera(QWidget *parent = nullptr);
    ~Tab1_camera();

    // ★ MainWidget에서 쓰는 getter 복구
    MotionDetector* detector() const { return m_detector; }

    // (원래 쓰던 슬롯 시그니처 유지)
public slots:
    void onFrameReady(const QImage& img, double clipLimit = 0.0);
    void onDetected();

private slots:
    void on_pPBCam_clicked();   // 네 UI 버튼(objectName: pPBCam) 토글

private:
    void updateView();

private:
    Ui::Tab1_camera *ui = nullptr;

    // ★ 원래 구조대로 Tab1이 Detector를 “소유”
    MotionDetector *m_detector = nullptr;

    QImage m_lastFrame;
    bool   m_displayOn = true;
};

#endif // TAB1_CAMERA_H
