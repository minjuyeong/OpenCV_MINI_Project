#ifndef TAB1_CAMERA_H
#define TAB1_CAMERA_H

#include <QWidget>
#include <QPointer>
#include <QKeyEvent>

#include "motiondetector.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Tab1_camera; }
QT_END_NAMESPACE

class Tab1_camera : public QWidget
{
    Q_OBJECT
public:
    explicit Tab1_camera(QWidget *parent = nullptr);
    ~Tab1_camera();

    // ✅ private -> public으로 이동하여 외부에서 접근 가능하도록 수정
    MotionDetector* detector() const { return m_detector; }

protected:
    void keyPressEvent(QKeyEvent *event) override;
    // ✅ resizeEvent 선언을 추가하여 cpp 파일의 구현과 일치시킴
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void onToggleDisplay();
    void onFrameReady(const QImage& img, double clipLimit);
    void onDetected();
    void onDetectionCleared();

private:
    void ensureCamLabel();
    void setDisplayEnabled(bool enable);
    void showPopup();
    void closePopup();

private:
    Ui::Tab1_camera *ui;
    class QLabel *m_camLabel = nullptr;
    class QLabel *m_badge = nullptr;
    bool m_showing = false;
    QPointer<class QMessageBox> m_alertBox;
    MotionDetector *m_detector = nullptr;
    QImage m_lastFrame;
    bool m_isAlertActive = false;
    bool m_autoClaheEnabled = true;
};

#endif // TAB1_CAMERA_H

