#ifndef MOTIONDETECTOR_H
#define MOTIONDETECTOR_H

#include <QObject>
#include <QImage>
#include <QThread>
#include <QString>
#include <atomic>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <QFuture>

class MotionDetector : public QObject
{
    Q_OBJECT
public:
    explicit MotionDetector(int camIndex = 0, QObject* parent = nullptr);
    ~MotionDetector();

    // 설정
    void setOutputDirectory(const QString& dir);   // 기본: ~/Videos/cctv
    void setRecordingSeconds(int sec);             // 기본: 8초, 이제 최소 녹화 시간으로 사용됨
    void setCameraIndex(int idx);

signals:
    void frameReady(const QImage& img);  // UI 표시용
    void detected();                     // 감지 신호(팝업 등)
    void detectionCleared(); // [추가] 위험 해제 신호
    void errorOccured(const QString& msg);

public slots:
    void start();
    void stop();

private:
    void runLoop();

    // 카메라 열기 (V4L2 우선 + /dev/video* 자동 탐색)
    bool openBestCamera();

    // 녹화 제어
    void startRecording();
    void stopRecording();

    static QImage matToQImage(const cv::Mat& bgr);

private:
    // 구성/상태
    int                  m_camIndex = 0;
    QString              m_outDir;              // 저장 폴더
    int                  m_recSeconds = 8;      //최소 녹화 시간

    // 스레드/루프
    QThread              m_worker;
    std::atomic_bool     m_running{false};

    // OpenCV
    cv::VideoCapture     m_cap;
    cv::VideoWriter      m_writer;
    bool                 m_recording = false;
    double               m_fps = 30.0;
    cv::Size             m_frameSize;

    // 워밍업/무시 마스크
    bool                                 m_armed = false;
    std::chrono::steady_clock::time_point m_tStart;
    cv::Mat                               m_ignoreMask; // 초기 장면 누적 마스크

    std::chrono::steady_clock::time_point m_lastDetectTime; // [추가] 마지막 감지 시간

    // 타임스탬프
    std::chrono::steady_clock::time_point m_recStarted;
    // private:
    int                  m_missCount = 0; //위험감지 팝업 노이즈 필터

    bool m_cameraReady = false;  // 첫 프레임 성공적으로 읽은 뒤 true

};

#endif // MOTIONDETECTOR_H
