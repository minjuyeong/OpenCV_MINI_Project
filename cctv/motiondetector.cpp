#include "motiondetector.h"

#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QMetaObject>
#include <QFileInfo>

using namespace std::chrono;

// ---- 파라미터 (필요시 조절) ----
namespace {
constexpr int   THRESH_BIN    = 150;      // 이진화 임계
constexpr int   MIN_AREA      = 1200;     // 최소 면적
constexpr int   ERODE_ITERS   = 0;
constexpr int   DILATE_ITERS  = 2;

// 워밍업/무시 마스크
constexpr int   WARMUP_MS     = 3000;     // 3초 워밍업
constexpr int   IGN_DILATE_K  = 21;       // 누적 마스크 팽창 커널
constexpr int   IGN_TRIM_K    = 15;       // 워밍업 종료 시 마스크 다듬기(침식)
}

MotionDetector::MotionDetector(int camIndex, QObject* parent)
    : QObject(parent), m_camIndex(camIndex)
{
    // 기본 저장 폴더
    m_outDir = QDir::homePath() + "/Videos/cctv";

    // 워커 시작 시 루프 연결
    connect(&m_worker, &QThread::started, this, &MotionDetector::runLoop);
}

MotionDetector::~MotionDetector()
{
    stop();
}

void MotionDetector::setOutputDirectory(const QString& dir)
{
    m_outDir = dir;
}

void MotionDetector::setRecordingSeconds(int sec)
{
    if (sec > 0) m_recSeconds = sec;
}

void MotionDetector::setCameraIndex(int idx)
{
    m_camIndex = idx;
}

void MotionDetector::start()
{
    if (m_running) return;

    // parent가 있으면 moveToThread가 막힐 수 있음 → 가능하면 nullptr로 생성
    if (parent() != nullptr) {
        qWarning() << "[MotionDetector] WARNING: parent is set. moveToThread may fail.";
    }

    // 객체 자체를 워커 스레드로 이동
    bool moved = (this->thread() == &m_worker) || this->moveToThread(&m_worker);
    if (!moved) {
        emit errorOccured(QStringLiteral("Failed to move detector to worker thread."));
        return;
    }

    m_running = true;
    m_worker.start();
}

void MotionDetector::stop()
{
    m_running = false;

    if (m_worker.isRunning()) {
        m_worker.quit();
        m_worker.wait();
    }

    stopRecording();
    if (m_cap.isOpened()) m_cap.release();
}

QImage MotionDetector::matToQImage(const cv::Mat& bgr)
{
    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
                  QImage::Format_RGB888).copy();
}

bool MotionDetector::openBestCamera()
{
    auto tryOpenByIndex = [this](int idx)->bool {
        if (m_cap.open(idx, cv::CAP_V4L2)) return true;
        if (m_cap.open(idx, cv::CAP_ANY))  return true;
        return false;
    };
    auto tryOpenByPath = [this](const std::string& path)->bool {
        if (m_cap.open(path, cv::CAP_V4L2)) return true;
        if (m_cap.open(path, cv::CAP_ANY))  return true;
        return false;
    };

    // 1) 지정 인덱스(/dev/videoX 존재 시 경로 우선)
    if (m_camIndex >= 0) {
        QString prefer = QString("/dev/video%1").arg(m_camIndex);
        if (QFileInfo::exists(prefer)) {
            if (tryOpenByPath(prefer.toStdString())) return true;
        }
        if (tryOpenByIndex(m_camIndex)) return true;
    }

    // 2) /dev/video[0..9] 경로 탐색
    for (int i=0; i<10; ++i) {
        QString dev = QString("/dev/video%1").arg(i);
        if (!QFileInfo::exists(dev)) continue;
        if (tryOpenByPath(dev.toStdString())) return true;
    }
    // 3) 인덱스 폴백
    for (int i=0; i<10; ++i) {
        if (tryOpenByIndex(i)) return true;
    }
    return false;
}

void MotionDetector::startRecording()
{
    if (m_recording) return;
    qDebug() << "[MotionDetector] startRecording() called (armed=" << m_armed
             << ", camReady=" << m_cameraReady << ")";

    QDir().mkpath(m_outDir);
    const QString ts   = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    const QString path = m_outDir + QString("/detect_%1.mp4").arg(ts);

    if (m_fps < 1.0) m_fps = 30.0;
    if (m_frameSize.width <= 0 || m_frameSize.height <= 0) m_frameSize = cv::Size(1280, 720);

    int fourcc = cv::VideoWriter::fourcc('m','p','4','v'); // mp4
    m_writer.release();
    if (!m_writer.open(path.toStdString(), fourcc, m_fps, m_frameSize, /*isColor*/true)) {
        emit errorOccured(QStringLiteral("VideoWriter open failed: %1").arg(path));
        return;
    }
    m_recording = true;
    m_recStarted = steady_clock::now();
    qDebug() << "[MotionDetector] Recording started:" << path;
}

void MotionDetector::stopRecording()
{
    if (!m_recording) return;
    m_writer.release();
    m_recording = false;
    qDebug() << "[MotionDetector] Recording stopped";
}

void MotionDetector::runLoop()
{
    if (!openBestCamera()) {
        emit errorOccured(QStringLiteral("Camera open failed."));
        m_running = false;
        return;
    }

    auto mog2 = cv::createBackgroundSubtractorMOG2(500, 16.0, true);

    m_armed = false;
    m_cameraReady = false;
    m_tStart = steady_clock::now();
    m_ignoreMask.release();

    cv::Mat frame, fg;

    // 첫 프레임 읽기
    if (m_cap.read(frame) && !frame.empty()) {
        m_cameraReady = true;
        // ★★★ 변경: invokeMethod 대신 즉시 신호 발행
        QImage q0 = matToQImage(frame);
        emit frameReady(q0);
    } else {
        emit errorOccured(QStringLiteral("Camera opened but first frame read failed."));
        m_running = false;
        stopRecording();
        if (m_cap.isOpened()) m_cap.release();
        return;
    }

    while (m_running) {
        if (!m_cap.read(frame) || frame.empty()) continue;

        double lr = m_armed ? 0.002 : 0.01;
        mog2->apply(frame, fg, lr);

        cv::threshold(fg, fg, THRESH_BIN, 255, cv::THRESH_BINARY);

        if (!m_armed) {
            if (m_ignoreMask.empty()) m_ignoreMask = cv::Mat::zeros(fg.size(), CV_8UC1);
            cv::Mat add;
            cv::dilate(fg, add, cv::getStructuringElement(cv::MORPH_ELLIPSE, {IGN_DILATE_K, IGN_DILATE_K}));
            cv::bitwise_or(m_ignoreMask, add, m_ignoreMask);

            if (duration_cast<milliseconds>(steady_clock::now() - m_tStart).count() >= WARMUP_MS) {
                cv::erode(m_ignoreMask, m_ignoreMask,
                          cv::getStructuringElement(cv::MORPH_ELLIPSE, {IGN_TRIM_K, IGN_TRIM_K}),
                          {-1,-1}, 1);
                m_armed = true;
                qDebug() << "[MotionDetector] armed. ignoreMask fixed.";
            }
        } else {
            if (!m_ignoreMask.empty()) {
                cv::Mat inv; cv::bitwise_not(m_ignoreMask, inv);
                cv::bitwise_and(fg, inv, fg);
            }
        }

        bool detectedNow = false;
        if (m_armed) {
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(fg, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            for (auto& c : contours) {
                if (cv::contourArea(c) > MIN_AREA) { detectedNow = true; break; }
            }
        }

        // ★★★ 변경: invokeMethod 대신 즉시 신호 발행
        if (m_cameraReady && m_armed && detectedNow) {
            emit detected();
            if (!m_recording) startRecording();
        }

        if (m_recording) {
            if (frame.size() != m_frameSize) {
                cv::Mat resized; cv::resize(frame, resized, m_frameSize);
                m_writer.write(resized);
            } else {
                m_writer.write(frame);
            }
            if (duration_cast<seconds>(steady_clock::now() - m_recStarted).count() >= m_recSeconds) {
                stopRecording();
            }
        }

        // ★★★ 변경: invokeMethod 대신 즉시 신호 발행
        QImage qimg = matToQImage(frame);
        emit frameReady(qimg);
    }

    stopRecording();
    if (m_cap.isOpened()) m_cap.release();
}
