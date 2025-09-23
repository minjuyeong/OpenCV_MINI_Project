#ifndef MOTIONDETECTOR_H
#define MOTIONDETECTOR_H

#include <QObject>
#include <QThread>
#include <QImage>
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

    // 설정 함수
    void setOutputDirectory(const QString& dir);
    void setRecordingSeconds(int sec);
    void setCameraIndex(int idx);

    // 영상 처리 옵션 설정 함수
    void setClaheEnabled(bool enabled);
    void setClaheParams(double clipLimit, int gridWidth, int gridHeight);
    void setMog2Params(int history, double varThreshold);

public slots:
    void start();
    void stop();

signals:
    void frameReady(const QImage& frame);
    void detected();
    void errorOccured(const QString& errorMsg);

private:
    void runLoop();
    bool openBestCamera();
    void startRecording();
    void stopRecording();
    QImage matToQImage(const cv::Mat& bgr);

    // 스레드 및 루프 제어
    QThread m_worker;
    std::atomic<bool> m_running = false;

    // 카메라 및 녹화 설정
    int m_camIndex;
    QString m_outDir;
    int m_recSeconds = 10;
    cv::VideoCapture m_cap;
    cv::VideoWriter m_writer;
    double m_fps = 0.0;
    cv::Size m_frameSize;

    // 상태 변수
    bool m_cameraReady = false;
    bool m_recording = false;
    bool m_armed = false;
    std::chrono::steady_clock::time_point m_tStart;
    std::chrono::steady_clock::time_point m_recStarted;
    bool m_motionInProgress = false; // 현재 움직임이 진행 중인지 상태를 저장

    // 영상 처리 관련
    cv::Mat m_ignoreMask;

    // 영상 처리 파라미터
    bool m_useClahe = false;
    double m_claheClipLimit = 2.0;
    cv::Size m_claheGridSize = cv::Size(8, 8);

    int m_mog2History = 500;
    double m_mog2VarThreshold = 16.0;
};

#endif // MOTIONDETECTOR_H