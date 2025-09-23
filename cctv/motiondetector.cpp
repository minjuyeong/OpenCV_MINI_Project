#include "motiondetector.h"

#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QMetaObject>
#include <QFileInfo>

using namespace std::chrono;

// ---- 파라미터 (필요시 조절) ----
namespace {
// 이진화 및 윤곽선 검출
constexpr int   THRESH_BIN      = 150;    // 이진화 임계
constexpr int   MIN_AREA        = 1200;   // 최소 면적

// 워밍업/무시 마스크
constexpr int   WARMUP_MS       = 3000;   // 3초 워밍업
constexpr int   IGN_DILATE_K    = 21;     // 누적 마스크 팽창 커널
constexpr int   IGN_TRIM_K      = 15;     // 워밍업 종료 시 마스크 다듬기(침식)

// MOG2 학습률
constexpr double LR_ARMED       = 0.002;  // 안정 상태 학습률
constexpr double LR_WARMUP      = 0.01;   // 워밍업 시 학습률
}

MotionDetector::MotionDetector(int camIndex, QObject* parent)
    : QObject(parent), m_camIndex(camIndex)
{
    m_outDir = QDir::homePath() + "/Videos/cctv";
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

void MotionDetector::setClaheEnabled(bool enabled)
{
    m_useClahe = enabled;
}

void MotionDetector::setClaheParams(double clipLimit, int gridWidth, int gridHeight)
{
    if (clipLimit > 0) m_claheClipLimit = clipLimit;
    if (gridWidth > 0 && gridHeight > 0) m_claheGridSize = cv::Size(gridWidth, gridHeight);
}

void MotionDetector::setMog2Params(int history, double varThreshold)
{
    if (history > 0) m_mog2History = history;
    if (varThreshold > 0) m_mog2VarThreshold = varThreshold;
}

void MotionDetector::start()
{
    if (m_running) return;
    if (parent() != nullptr) {
        qWarning() << "[MotionDetector] WARNING: parent is set. moveToThread may fail.";
    }
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

    if (m_camIndex >= 0) {
        QString prefer = QString("/dev/video%1").arg(m_camIndex);
        if (QFileInfo::exists(prefer)) {
            if (tryOpenByPath(prefer.toStdString())) return true;
        }
        if (tryOpenByIndex(m_camIndex)) return true;
    }

    for (int i=0; i<10; ++i) {
        QString dev = QString("/dev/video%1").arg(i);
        if (!QFileInfo::exists(dev)) continue;
        if (tryOpenByPath(dev.toStdString())) return true;
    }
    for (int i=0; i<10; ++i) {
        if (tryOpenByIndex(i)) return true;
    }
    return false;
}

void MotionDetector::startRecording()
{
    if (m_recording) return;
    qDebug() << "[MotionDetector] startRecording() called";

    QDir().mkpath(m_outDir);
    const QString ts   = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    const QString path = m_outDir + QString("/detect_%1.mp4").arg(ts);

    if (m_fps < 1.0) m_fps = 30.0;
    if (m_frameSize.width <= 0 || m_frameSize.height <= 0) m_frameSize = cv::Size(1280, 720);

    int fourcc = cv::VideoWriter::fourcc('m','p','4','v');
    m_writer.release();
    if (!m_writer.open(path.toStdString(), fourcc, m_fps, m_frameSize, true)) {
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

    auto mog2 = cv::createBackgroundSubtractorMOG2(m_mog2History, m_mog2VarThreshold, true);

    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(m_claheClipLimit);
    clahe->setTilesGridSize(m_claheGridSize);

    m_armed = false;
    m_cameraReady = false;
    m_motionInProgress = false;
    m_tStart = steady_clock::now();
    m_ignoreMask.release();

    cv::Mat frame, fg;
    if (m_cap.read(frame) && !frame.empty()) {
        m_cameraReady = true;
        m_fps = m_cap.get(cv::CAP_PROP_FPS);
        m_frameSize = frame.size();
        emit frameReady(matToQImage(frame));
    } else {
        emit errorOccured(QStringLiteral("Camera opened but first frame read failed."));
        m_running = false;
        stopRecording();
        if (m_cap.isOpened()) m_cap.release();
        return;
    }

    while (m_running) {
        if (!m_cap.read(frame) || frame.empty()) continue;

        if (m_useClahe) {
            cv::Mat lab_image;
            cv::cvtColor(frame, lab_image, cv::COLOR_BGR2Lab);
            std::vector<cv::Mat> lab_planes(3);
            cv::split(lab_image, lab_planes);
            clahe->apply(lab_planes[0], lab_planes[0]);
            cv::merge(lab_planes, lab_image);
            cv::cvtColor(lab_image, frame, cv::COLOR_Lab2BGR);
        }

        double lr = m_armed ? LR_ARMED : LR_WARMUP;
        mog2->apply(frame, fg, lr);

        cv::threshold(fg, fg, THRESH_BIN, 255, cv::THRESH_BINARY);

        if (!m_armed) {
            if (m_ignoreMask.empty()) m_ignoreMask = cv::Mat::zeros(fg.size(), CV_8UC1);
            cv::Mat add;
            cv::dilate(fg, add, cv::getStructuringElement(cv::MORPH_ELLIPSE, {IGN_DILATE_K, IGN_DILATE_K}));
            cv::bitwise_or(m_ignoreMask, add, m_ignoreMask);

            if (duration_cast<milliseconds>(steady_clock::now() - m_tStart).count() >= WARMUP_MS) {
                cv::erode(m_ignoreMask, m_ignoreMask,
                          cv::getStructuringElement(cv::MORPH_ELLIPSE, {IGN_TRIM_K, IGN_TRIM_K}));
                m_armed = true;
                qDebug() << "[MotionDetector] armed. ignoreMask fixed.";
            }
        } else {
            if (!m_ignoreMask.empty()) {
                cv::bitwise_and(fg, m_ignoreMask, fg, cv::noArray());
            }
        }

        bool detectedNow = false;
        if (m_armed) {
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(fg, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            for (auto& c : contours) {
                if (cv::contourArea(c) > MIN_AREA) {
                    detectedNow = true;
                    break;
                }
            }
        }

        // 팝업 중복 방지 로직
        if (detectedNow && !m_motionInProgress) {
            m_motionInProgress = true;
            emit detected();
            if (!m_recording) {
                startRecording();
            }
        } else if (!detectedNow && m_motionInProgress) {
            m_motionInProgress = false;
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

        emit frameReady(matToQImage(frame));
    }

    stopRecording();
    if (m_cap.isOpened()) m_cap.release();
}