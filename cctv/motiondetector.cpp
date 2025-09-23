#include "motiondetector.h"

#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

using namespace std::chrono;

namespace {
constexpr int   THRESH_BIN    = 150;
constexpr int   MIN_AREA      = 1200;
constexpr int   ERODE_ITERS   = 0;
constexpr int   DILATE_ITERS  = 2;

constexpr int   WARMUP_MS     = 3000;
constexpr int   IGN_DILATE_K  = 21;
constexpr int   IGN_TRIM_K    = 15;

constexpr int   REC_GRACE_PERIOD_S = 5;
}
constexpr double LR_ARMED  = 0.002;
constexpr double LR_WARMUP = 0.01;

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

void MotionDetector::setOutputDirectory(const QString& dir) { m_outDir = dir; }
void MotionDetector::setRecordingSeconds(int sec) { if (sec > 0) m_recSeconds = sec; }
void MotionDetector::setCameraIndex(int idx) { m_camIndex = idx; }
void MotionDetector::setClaheEnabled(bool enabled) { m_useClahe = enabled; }

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
    // 내부캠 폴백 없이 URL만 사용
    if (m_camUrl.isEmpty()) {
        qWarning() << "[MotionDetector] m_camUrl is empty. Set CameraUrl first.";
        return false;
    }

    auto tryOpenByPath = [this](const std::string& path)->bool {
        // FFmpeg 우선(HTTP/RTSP/MJPEG/파일 경로)
        if (m_cap.open(path, cv::CAP_FFMPEG)) return true;
        if (m_cap.open(path, cv::CAP_ANY))    return true;
        return false;
    };

    qDebug() << "[MotionDetector] Trying URL:" << m_camUrl;
    return tryOpenByPath(m_camUrl.toStdString());
}

void MotionDetector::startRecording()
{
    if (m_recording) return;

    QDir().mkpath(m_outDir);
    const QString ts   = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    const QString path = m_outDir + QString("/detect_%1.mp4").arg(ts);

    if (m_fps < 1.0 || !std::isfinite(m_fps)) m_fps = 30.0;
    if (m_frameSize.width <= 0 || m_frameSize.height <= 0) m_frameSize = cv::Size(1280, 720);

    int fourcc = cv::VideoWriter::fourcc('m','p','4','v'); // mp4
    m_writer.release();
    if (!m_writer.open(path.toStdString(), fourcc, m_fps, m_frameSize, true)) {
        emit errorOccured(QStringLiteral("VideoWriter open failed: %1").arg(path));
        return;
    }
    m_recording  = true;
    m_recStarted = steady_clock::now();
    qDebug() << "[MotionDetector] Recording started:" << path
             << "fps=" << m_fps << "size=" << m_frameSize.width << "x" << m_frameSize.height;
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
    m_tStart = steady_clock::now();
    m_ignoreMask.release();

    cv::Mat frame, fg;

    // 첫 프레임 읽기
    if (m_cap.read(frame) && !frame.empty()) {
        m_cameraReady = true;

        double fpsProbe = m_cap.get(cv::CAP_PROP_FPS);
        if (fpsProbe > 0.0 && std::isfinite(fpsProbe)) m_fps = fpsProbe;
        m_frameSize = frame.size();

        emit frameReady(matToQImage(frame), 0.0);
    } else {
        emit errorOccured(QStringLiteral("Camera opened but first frame read failed."));
        m_running = false;
        stopRecording();
        if (m_cap.isOpened()) m_cap.release();
        return;
    }

    bool wasDetectingLastFrame = false;

    while (m_running) {
        cv::Mat frame;
        if (!m_cap.read(frame) || frame.empty()) continue;

        // --- 자동 밝기 감지 ---
        bool   applyClaheThisFrame = m_useClahe;
        double currentClipLimit    = m_claheClipLimit;

        if (m_autoClahe && !applyClaheThisFrame) {
            cv::Mat gray;
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            double brightness = cv::mean(gray)[0];
            if (brightness < m_darknessThreshold) {
                applyClaheThisFrame = true;
                currentClipLimit = m_claheMaxClip - (m_claheMaxClip - 1.0) * (brightness / m_darknessThreshold);
                clahe->setClipLimit(currentClipLimit);
                qDebug() << "Brightness:" << brightness << "-> ClipLimit:" << currentClipLimit;
            }
        }

        // --- 영상 처리 ---
        cv::Mat processedFrame;
        if (applyClaheThisFrame) {
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

        // --- 모션 감지 ---
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
                          cv::getStructuringElement(cv::MORPH_ELLIPSE, {IGN_TRIM_K, IGN_TRIM_K}),
                          {-1,-1}, 1);
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
                if (cv::contourArea(c) > MIN_AREA) { detectedNow = true; break; }
            }
        }

        // 감지/녹화 제어
        if (m_cameraReady && m_armed) {
            if (detectedNow) {
                m_missCount = 0;
                if (!wasDetectingLastFrame) emit detected();
                if (!m_recording) startRecording();
                m_lastDetectTime = steady_clock::now();
                wasDetectingLastFrame = true;
            } else {
                m_missCount++;
                if (m_missCount >= 8 && wasDetectingLastFrame) {
                    emit detectionCleared();
                    wasDetectingLastFrame = false;
                    m_missCount = 0;
                }
            }
        } else if (!detectedNow && m_motionInProgress) {
            m_motionInProgress = false;
        }

        // 녹화 중이면 처리된 프레임 저장
        if (m_recording) {
            if (processedFrame.size() != m_frameSize) {
                cv::Mat resized; cv::resize(processedFrame, resized, m_frameSize);
                m_writer.write(resized);
            } else {
                m_writer.write(processedFrame);
            }
            auto now = steady_clock::now();
            bool gracePassed = duration_cast<seconds>(now - m_lastDetectTime).count() >= REC_GRACE_PERIOD_S;
            bool minPassed   = duration_cast<seconds>(now - m_recStarted).count() >= m_recSeconds;
            if (gracePassed && minPassed) stopRecording();
        }

        emit frameReady(matToQImage(processedFrame), applyClaheThisFrame ? currentClipLimit : 0.0);
    }

    stopRecording();
    if (m_cap.isOpened()) m_cap.release();
}

void MotionDetector::setAutoClaheEnabled(bool enabled) { m_autoClahe = enabled; }

void MotionDetector::setAutoClaheParams(int darknessThreshold, double maxClip)
{
    if (darknessThreshold > 0 && darknessThreshold <= 255) m_darknessThreshold = darknessThreshold;
    if (maxClip > 0) m_claheMaxClip = maxClip;
}
