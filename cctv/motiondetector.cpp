#include "motiondetector.h"

#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QMetaObject>
#include <QFileInfo>

using namespace std::chrono;

// ---- Parameters (Adjust if needed) ----
namespace {
constexpr int    THRESH_BIN         = 150;    // Binarization threshold
constexpr int    MIN_AREA           = 1200;   // Minimum contour area
constexpr int    WARMUP_MS          = 3000;   // 3-second warm-up
constexpr int    IGN_DILATE_K       = 21;     // Dilation kernel for ignore mask
constexpr int    IGN_TRIM_K         = 15;     // Erosion kernel for trimming mask
constexpr int    REC_GRACE_PERIOD_S = 5;      // Record for 5 more seconds after detection stops
}
// MOG2 Learning Rates
constexpr double LR_ARMED           = 0.002;  // Learning rate when armed
constexpr double LR_WARMUP          = 0.01;   // Learning rate during warm-up

MotionDetector::MotionDetector(int camIndex, QObject* parent)
    : QObject(parent), m_camIndex(camIndex)
{
    m_outDir = QDir::homePath() + "/Videos/cctv";
    connect(&m_worker, &QThread::started, this, &MotionDetector::runLoop);
}

MotionDetector::~MotionDetector()
{
    stop();
    // ✅ [수정] 프로그램 종료 시 녹화 중인 파일이 있다면 확실히 마무리합니다.
    if (m_writer.isOpened()) {
        m_writer.release();
        qDebug() << "[MotionDetector] Final writer release on exit.";
    }
}

void MotionDetector::setOutputDirectory(const QString& dir) { m_outDir = dir; }
void MotionDetector::setRecordingSeconds(int sec) { if (sec > 0) m_recSeconds = sec; }
void MotionDetector::setCameraIndex(int idx) { m_camIndex = idx; }
void MotionDetector::setClaheEnabled(bool enabled) { m_useClahe = enabled; }
void MotionDetector::setClaheParams(double clipLimit, int gridWidth, int gridHeight) {
    if (clipLimit > 0) m_claheClipLimit = clipLimit;
    if (gridWidth > 0 && gridHeight > 0) m_claheGridSize = cv::Size(gridWidth, gridHeight);
}
void MotionDetector::setMog2Params(int history, double varThreshold) {
    if (history > 0) m_mog2History = history;
    if (varThreshold > 0) m_mog2VarThreshold = varThreshold;
}
void MotionDetector::setAutoClaheEnabled(bool enabled) { m_autoClahe = enabled; }
void MotionDetector::setAutoClaheParams(int darknessThreshold, double maxClip) {
    if (darknessThreshold > 0 && darknessThreshold <= 255) m_darknessThreshold = darknessThreshold;
    if (maxClip > 0) m_claheMaxClip = maxClip;
}

void MotionDetector::start()
{
    if (m_running) return;
    if (parent() != nullptr) {
        qWarning() << "[MotionDetector] WARNING: parent is set. moveToThread may fail.";
    }
    if (!moveToThread(&m_worker)) {
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
    return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
}

bool MotionDetector::openBestCamera()
{
    const std::string streamUrl = "http://10.10.16.63:8080/?action=stream";
    qDebug() << "[MotionDetector] Opening network stream:" << QString::fromStdString(streamUrl);

    // FFMPEG 백엔드가 네트워크 스트림에 더 안정적인 경우가 많음
    if (m_cap.open(streamUrl, cv::CAP_FFMPEG)) {
        qDebug() << "[MotionDetector] Stream opened successfully with FFMPEG backend.";
        return true;
    }

    qDebug() << "[MotionDetector] Failed to open stream with FFMPEG backend, trying ANY backend...";
    if (m_cap.open(streamUrl, cv::CAP_ANY)) {
        qDebug() << "[MotionDetector] Stream opened successfully with ANY backend.";
        return true;
    }

    qDebug() << "[MotionDetector] Failed to open stream with all backends.";
    return false;
}

void MotionDetector::startRecording()
{
    if (m_recording) return;

    QDir().mkpath(m_outDir);
    const QString path = m_outDir + "/detect_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".mp4";

    if (m_fps < 1.0) m_fps = 30.0;
    if (m_frameSize.empty()) m_frameSize = cv::Size(1280, 720);

    // ✅ [수정] 코덱을 'mp4v'에서 'avc1' (H.264)으로 변경합니다.
    int fourcc = cv::VideoWriter::fourcc('a','v','c','1');
    m_writer.release(); // 이전 writer가 열려있을 수 있으므로 먼저 닫음
    if (!m_writer.open(path.toStdString(), fourcc, m_fps, m_frameSize, true)) {
        emit errorOccured(QStringLiteral("VideoWriter open failed: %1").arg(path));
        return;
    }
    m_recording = true;
    m_recStarted = std::chrono::steady_clock::now();
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
    clahe->setTilesGridSize(m_claheGridSize);

    m_armed = false;
    m_cameraReady = false;
    m_motionInProgress = false;
    m_tStart = std::chrono::steady_clock::now();
    m_ignoreMask.release();

    cv::Mat frame;
    if (m_cap.read(frame) && !frame.empty()) {
        m_cameraReady = true;
        m_fps = m_cap.get(cv::CAP_PROP_FPS);
        m_frameSize = frame.size();
        emit frameReady(matToQImage(frame), 0.0);
    } else {
        emit errorOccured(QStringLiteral("Camera opened but first frame read failed."));
        m_running = false;
        return;
    }

    while (m_running) {
        if (!m_cap.read(frame) || frame.empty()) continue;

        bool applyClaheThisFrame = m_useClahe;
        double currentClipLimit = m_claheClipLimit;

        if (m_autoClahe) {
            cv::Mat gray;
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            double brightness = cv::mean(gray)[0];
            if (brightness < m_darknessThreshold) {
                applyClaheThisFrame = true;
                currentClipLimit = m_claheMaxClip - (m_claheMaxClip - 1.0) * (brightness / m_darknessThreshold);
            } else {
                applyClaheThisFrame = false;
            }
        }

        cv::Mat processedFrame;
        if (applyClaheThisFrame) {
            clahe->setClipLimit(currentClipLimit);
            cv::Mat lab_image;
            cv::cvtColor(frame, lab_image, cv::COLOR_BGR2Lab);
            std::vector<cv::Mat> lab_planes(3);
            cv::split(lab_image, lab_planes);
            clahe->apply(lab_planes[0], lab_planes[0]);
            cv::merge(lab_planes, lab_image);
            cv::cvtColor(lab_image, processedFrame, cv::COLOR_Lab2BGR);
        } else {
            frame.copyTo(processedFrame);
        }

        cv::Mat fg;
        mog2->apply(processedFrame, fg, m_armed ? LR_ARMED : LR_WARMUP);
        cv::threshold(fg, fg, THRESH_BIN, 255, cv::THRESH_BINARY);

        if (!m_armed) {
            if (m_ignoreMask.empty()) m_ignoreMask = cv::Mat::zeros(fg.size(), CV_8UC1);
            cv::dilate(fg, fg, cv::getStructuringElement(cv::MORPH_ELLIPSE, {IGN_DILATE_K, IGN_DILATE_K}));
            cv::bitwise_or(m_ignoreMask, fg, m_ignoreMask);
            if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_tStart).count() >= WARMUP_MS) {
                cv::erode(m_ignoreMask, m_ignoreMask, cv::getStructuringElement(cv::MORPH_ELLIPSE, {IGN_TRIM_K, IGN_TRIM_K}));
                m_armed = true;
                qDebug() << "[MotionDetector] armed. ignoreMask fixed.";
            }
        } else {
            if(!m_ignoreMask.empty()) cv::bitwise_and(fg, m_ignoreMask, fg, cv::noArray());
        }

        bool detectedNow = false;
        if (m_armed) {
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(fg, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            for (const auto& c : contours) {
                if (cv::contourArea(c) > MIN_AREA) {
                    detectedNow = true;
                    break;
                }
            }
        }

        if (detectedNow && !m_motionInProgress) {
            m_motionInProgress = true;
            emit detected();
            if (!m_recording) startRecording();
        } else if (!detectedNow && m_motionInProgress) {
            m_motionInProgress = false;
            emit detectionCleared();
        }
        if(detectedNow) m_lastDetectTime = std::chrono::steady_clock::now();

        if (m_recording) {
            m_writer.write(processedFrame);
            auto now = std::chrono::steady_clock::now();
            bool gracePeriodPassed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastDetectTime).count() >= REC_GRACE_PERIOD_S;
            bool minRecTimePassed = std::chrono::duration_cast<std::chrono::seconds>(now - m_recStarted).count() >= m_recSeconds;
            if (gracePeriodPassed && minRecTimePassed) {
                stopRecording();
            }
        }

        emit frameReady(matToQImage(processedFrame), applyClaheThisFrame ? currentClipLimit : 0.0);
    }

    stopRecording();
    if (m_cap.isOpened()) m_cap.release();
}

