#include "tab2_video.h"
#include "ui_tab2_video.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QMessageBox>
#include <QUrl>

#include <QMediaPlayer>
#include <QVideoWidget>
#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <opencv2/opencv.hpp>
#include <QMap>
#include <QDate>
#include <QDebug>
#include <QFuture>
#include <QtConcurrent>

// 유틸
static bool isVideoFile(const QString& fn) {
    const QString ext = QFileInfo(fn).suffix().toLower();
    static const QStringList ok = {"mp4","mov","m4v","avi","mkv","wmv"};
    return ok.contains(ext);
}

Tab2_video::Tab2_video(const QString& mediaDir, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Tab2_video)
    , m_mediaDir(mediaDir)
{
    ui->setupUi(this);

    if (m_mediaDir.isEmpty()) {
        m_mediaDir = QDir::homePath() + "/Videos/cctv";
    }
    QDir().mkpath(m_mediaDir);

    setupVideoOutput();

    connect(ui->btnRefresh, &QPushButton::clicked, this, &Tab2_video::refreshGallery);
    connect(ui->btnPlay,    &QPushButton::clicked, this, &Tab2_video::playSelected);
    connect(ui->list,       &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*){ playSelected(); });
    connect(ui->btnBack,    &QPushButton::clicked, this, &Tab2_video::backToGallery);
    connect(this, &Tab2_video::thumbnailReady, this, &Tab2_video::onThumbnailReady);

    refreshGallery();
    ui->stack->setCurrentIndex(0);
}

Tab2_video::Tab2_video(QWidget *parent)
    : Tab2_video(QString(), parent) {}

Tab2_video::Tab2_video(QWidget *parent, const QString& mediaDir)
    : Tab2_video(mediaDir, parent) {}

Tab2_video::~Tab2_video()
{
    if (m_player) m_player->stop();
    if (m_thumbFuture.isRunning()) {
        m_thumbFuture.cancel();
        m_thumbFuture.waitForFinished();
    }
    delete ui;
}

void Tab2_video::setupVideoOutput()
{
    m_player = new QMediaPlayer(this);
    m_video  = new QVideoWidget(ui->videoHost);
    m_video->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (auto lay = ui->videoHost->layout()) {
        lay->setContentsMargins(0,0,0,0);
        lay->addWidget(m_video);
    }
    m_player->setVideoOutput(m_video);
}

void Tab2_video::setMediaDirectory(const QString& dir)
{
    m_mediaDir = dir;
    QDir().mkpath(m_mediaDir);
    refreshGallery();
}

void Tab2_video::refreshGallery()
{
    if (m_thumbFuture.isRunning()) {
        m_thumbFuture.cancel();
        m_thumbFuture.waitForFinished();
    }

    ui->list->clear();
    ui->title->setText(QStringLiteral("영상 갤러리 — %1").arg(m_mediaDir));

    QDir dir(m_mediaDir);
    const auto entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);

    QMap<QDate, QList<QFileInfo>> groupedByDate;
    for (const QFileInfo& fi : entries) {
        if (!isVideoFile(fi.fileName())) continue;
        QStringList parts = fi.baseName().split('_');
        if (parts.size() < 2) continue;
        QDate date = QDate::fromString(parts.at(1), "yyyyMMdd");
        if (date.isValid()) {
            groupedByDate[date].append(fi);
        }
    }

    QMapIterator<QDate, QList<QFileInfo>> it(groupedByDate);
    it.toBack();

    while (it.hasPrevious()) {
        it.previous();
        const QDate& date = it.key();
        const QList<QFileInfo>& filesOnDate = it.value();

        auto *headerItem = new QListWidgetItem();
        headerItem->setText(date.toString("yyyy년 M월 d일 (dddd)"));
        headerItem->setFlags(headerItem->flags() & ~Qt::ItemIsSelectable);
        QFont font = headerItem->font();
        font.setBold(true); font.setPointSize(14);
        headerItem->setFont(font);
        headerItem->setBackground(QColor("#f0f0f0"));
        ui->list->addItem(headerItem);

        for (const QFileInfo& fi : filesOnDate) {
            auto *item = new QListWidgetItem();
            item->setText(fi.fileName());
            item->setToolTip(fi.absoluteFilePath());
            ui->list->addItem(item);
        }
    }

    // ✅ 람다 함수로 수정된 부분
    m_thumbFuture = QtConcurrent::run([this]() {
        this->loadThumbnailsInBackground();
    });
}

QIcon Tab2_video::makeThumbFor(const QString& filePath, QSize target)
{
    try {
        cv::VideoCapture cap(filePath.toStdString());
        if (!cap.isOpened()) {
            throw std::runtime_error("VideoCapture open failed.");
        }
        cv::Mat frame;
        for (int i=0; i<10; ++i) cap.read(frame);
        if (frame.empty()) cap.read(frame);
        cap.release();

        if (frame.empty()) {
            throw std::runtime_error("Failed to grab a frame.");
        }

        cv::Mat rgb;
        cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
        QImage qimg(rgb.data, rgb.cols, rgb.rows, (int)rgb.step, QImage::Format_RGB888);
        QImage scaled = qimg.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        return QIcon(QPixmap::fromImage(scaled));

    } catch (const std::exception& e) {
        qWarning() << "[Thumb Error] for" << filePath << ":" << e.what();
        QPixmap px(target);
        px.fill(Qt::black);
        QPainter p(&px);
        p.setPen(Qt::white);
        p.drawText(px.rect(), Qt::AlignCenter, "NO PREVIEW");
        p.end();
        return QIcon(px);
    }
}

void Tab2_video::playSelected()
{
    auto *item = ui->list->currentItem();
    if (!item || item->toolTip().isEmpty()) return; // 헤더 아이템은 재생 안 함

    const QString path = item->toolTip();
    if (!QFileInfo::exists(path)) {
        QMessageBox::warning(this, QStringLiteral("오류"),
                             QStringLiteral("파일을 찾을 수 없습니다:\n%1").arg(path));
        return;
    }

    ui->nowPlaying->setText(path);
    m_player->stop();
    m_player->setSource(QUrl::fromLocalFile(path));
    m_player->play();

    ui->stack->setCurrentIndex(1);
    ui->title->setText(QStringLiteral("재생 중"));
}

void Tab2_video::backToGallery()
{
    if (m_player) m_player->pause();
    ui->stack->setCurrentIndex(0);
    ui->title->setText(QStringLiteral("영상 갤러리 — %1").arg(m_mediaDir));
}

void Tab2_video::onDetected()
{
    // (생략)
}

void Tab2_video::centerAndRaise(QWidget *dlg)
{
    if (!dlg) return;
    QWidget *top = this->window();
    QScreen *scr = top ? top->screen() : QGuiApplication::primaryScreen();
    const QRect sr = scr ? scr->availableGeometry()
                         : QGuiApplication::primaryScreen()->availableGeometry();
    const QSize sz = dlg->sizeHint();
    dlg->move(sr.center() - QPoint(sz.width()/2, sz.height()/2));
    dlg->raise();
    dlg->activateWindow();
}

void Tab2_video::loadThumbnailsInBackground()
{
    for (int i = 0; i < ui->list->count(); ++i) {
        if (m_thumbFuture.isCanceled()) break;

        QListWidgetItem* item = ui->list->item(i);
        if (!item || item->toolTip().isEmpty()) continue;

        QIcon icon = makeThumbFor(item->toolTip());
        emit thumbnailReady(item, icon);
    }
}

void Tab2_video::onThumbnailReady(QListWidgetItem* item, const QIcon& icon)
{
    if (item) {
        item->setIcon(icon);
    }
}
