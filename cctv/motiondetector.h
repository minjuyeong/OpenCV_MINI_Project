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
    explicit MotionDetector(int camIndex = 0, QObject* parent = nullptr);
    ~MotionDetector();

    // 설정
    void setOutputDirectory(const QString& dir);
    void setRecordingSeconds(int sec);
    void setCameraIndex(int idx);

    // 영상 처리 옵션 설정 함수
    void setClaheEnabled(bool enabled);
    void setClaheParams(double clipLimit, int gridWidth, int gridHeight);
    void setMog2Params(int history, double varThreshold);
    void setAutoClaheEnabled(bool enabled);
    void setAutoClaheParams(int darknessThreshold, double maxClip);

signals:
    // ✅ 이 줄을 수정하여 double 인자를 추가합니다.
    void frameReady(const QImage& img, double clipLimit);

    void detected();
    void detectionCleared();
    void errorOccured(const QString& msg);

public slots:
    void start();
    void stop();

private:
    void runLoop();
    bool openBestCamera();
    void startRecording();
    void stopRecording();
    QImage matToQImage(const cv::Mat& bgr);

private:
    // (이하 멤버 변수들은 기존 코드와 동일)
    int               m_camIndex = 0;
    QString           m_outDir;
    int               m_recSeconds = 8;
    QThread           m_worker;
    std::atomic_bool  m_running{false};
    cv::VideoCapture  m_cap;
    cv::VideoWriter   m_writer;
    bool              m_recording = false;
    double            m_fps = 30.0;
    cv::Size          m_frameSize;
    bool m_cameraReady = false;
    bool m_armed = false;
    std::chrono::steady_clock::time_point m_tStart;
    std::chrono::steady_clock::time_point m_recStarted;
    cv::Mat           m_ignoreMask;
    bool m_motionInProgress = false;
    std::chrono::steady_clock::time_point m_lastDetectTime;
    int               m_missCount = 0;
    bool m_useClahe = false;
    bool m_autoClahe = false;
    int  m_darknessThreshold = 80;
    double m_claheMaxClip = 8.0;
    double m_claheClipLimit = 2.0;
    cv::Size m_claheGridSize = cv::Size(8, 8);
    int m_mog2History = 500;
    double m_mog2VarThreshold = 16.0;
};

#endif // MOTIONDETECTOR_H


