#ifndef MOTIONDETECTOR_H
#define MOTIONDETECTOR_H

#include <QObject>
#include <QImage>
#include <QThread>
#include <QString>
#include <atomic>
#include <opencv2/opencv.hpp>
#include <chrono>

class MotionDetector : public QObject
{
    Q_OBJECT
public:
    explicit MotionDetector(int camIndex = -1, QObject* parent = nullptr);
    ~MotionDetector();

    // 설정
    void setOutputDirectory(const QString& dir);   // 기본: ~/Videos/cctv
    void setRecordingSeconds(int sec);             // 기본: 8초(최소 녹화 시간)
    void setCameraIndex(int idx);                  // URL 미설정 시에만 의미 있음
    void setCameraUrl(const QString& url) { m_camUrl = url; } // ★ HTTP/RTSP 등 URL 입력

    // 영상 처리 옵션
    void setClaheEnabled(bool enabled);
    void setClaheParams(double clipLimit, int gridWidth, int gridHeight);
    void setMog2Params(int history, double varThreshold);

    void setAutoClaheEnabled(bool enabled);
    void setAutoClaheParams(int darknessThreshold, double maxClip);

signals:
    void frameReady(const QImage& img, double clipLimit);
    void detected();
    void detectionCleared();
    void errorOccured(const QString& msg);

public slots:
    void start();
    void stop();

private:
    void runLoop();

    // 카메라 열기 (URL만 사용, 실패 시 false 반환 — 내부캠 폴백 없음)
    bool openBestCamera();

    // 녹화 제어
    void startRecording();
    void stopRecording();

    static QImage matToQImage(const cv::Mat& bgr);

private:
    // 구성/상태
    int                  m_camIndex = -1;
    QString              m_camUrl;              // ★ 입력 소스 URL(비어있지 않아야 함)
    QString              m_outDir;              // 저장 폴더
    int                  m_recSeconds = 8;      // 최소 녹화 시간(초)

    // 스레드/루프
    QThread              m_worker;
    std::atomic_bool     m_running{false};

    // OpenCV
    cv::VideoCapture     m_cap;
    cv::VideoWriter      m_writer;
    bool                 m_recording = false;
    double               m_fps = 30.0;
    cv::Size             m_frameSize;

    // 상태 변수
    bool m_cameraReady = false;
    bool m_armed = false;

    // 워밍업/무시 마스크
    std::chrono::steady_clock::time_point m_tStart;
    std::chrono::steady_clock::time_point m_recStarted;
    cv::Mat                               m_ignoreMask;

    bool m_motionInProgress = false;
    std::chrono::steady_clock::time_point m_lastDetectTime;
    int  m_missCount = 0;

    // 영상 처리 파라미터
    bool   m_useClahe = false;
    bool   m_autoClahe = false;
    int    m_darknessThreshold = 80;
    double m_claheMaxClip = 8.0;

    double   m_claheClipLimit = 2.0;
    cv::Size m_claheGridSize  = cv::Size(8, 8);

    int     m_mog2History     = 500;
    double  m_mog2VarThreshold= 16.0;
};

#endif // MOTIONDETECTOR_H
