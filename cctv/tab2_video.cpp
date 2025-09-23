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
#include <QMap>     // QMap 사용을 위해 추가
#include <QDate>    // QDate 사용을 위해 추가

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

    // 기본 폴더
    if (m_mediaDir.isEmpty()) {
        m_mediaDir = QDir::homePath() + "/Videos/cctv";
    }
    QDir().mkpath(m_mediaDir);

    // 비디오 출력 구성
    setupVideoOutput();

    // 연결
    connect(ui->btnRefresh, &QPushButton::clicked, this, &Tab2_video::refreshGallery);
    connect(ui->btnPlay,    &QPushButton::clicked, this, &Tab2_video::playSelected);
    connect(ui->list,       &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*){ playSelected(); });
    connect(ui->btnBack,    &QPushButton::clicked, this, &Tab2_video::backToGallery);

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
    delete ui;
}

void Tab2_video::setupVideoOutput()
{
    // player & video widget
    m_player = new QMediaPlayer(this);
    m_video  = new QVideoWidget(ui->videoHost);
    m_video->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // videoHost의 레이아웃에 부착
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
    ui->list->clear();
    ui->title->setText(QStringLiteral("영상 갤러리 — %1").arg(m_mediaDir));

    QDir dir(m_mediaDir);
    //const auto entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot,
    //                                       QDir::Time | QDir::Reversed);

    // [수정] 정렬 기준을 이름(Name)으로 변경하여 파일 이름순으로 가져옵니다.
    const auto entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot,
                                           QDir::Name | QDir::Reversed);

    // 1. 파일 목록을 날짜(QDate)를 키로 사용하는 QMap으로 그룹화합니다.
    QMap<QDate, QList<QFileInfo>> groupedByDate;
    for (const QFileInfo& fi : entries) {
        if (!isVideoFile(fi.fileName())) continue;

        // 파일 이름 형식 'detect_yyyyMMdd_HHmmss.mp4' 에서 날짜 추출
        QStringList parts = fi.baseName().split('_');
        if (parts.size() < 2) continue; // 형식이 맞지 않으면 건너뜀

        QDate date = QDate::fromString(parts.at(1), "yyyyMMdd");
        if (date.isValid()) {
            // 해당 날짜의 목록에 현재 파일 정보를 추가합니다.
            groupedByDate[date].append(fi);
        }
    }

    // 2. 그룹화된 맵(Map)을 최신 날짜부터(역순) 순회합니다.
    QMapIterator<QDate, QList<QFileInfo>> it(groupedByDate);
    it.toBack(); // 반복자를 맨 뒤로 이동 (가장 최신 날짜)

    while (it.hasPrevious()) {
        it.previous();
        const QDate& date = it.key();
        const QList<QFileInfo>& filesOnDate = it.value();

        // 3. 날짜가 바뀔 때마다 '날짜 헤더' 아이템을 먼저 추가합니다.
        auto *headerItem = new QListWidgetItem();
        // 예: "2025년 9월 22일 (월요일)" 형식으로 변환
        headerItem->setText(date.toString("yyyy년 M월 d일 (dddd)"));

        // 헤더는 선택할 수 없도록 설정
        headerItem->setFlags(headerItem->flags() & ~Qt::ItemIsSelectable);

        // 헤더 스타일링 (굵게, 큰 글씨, 배경색)
        QFont font = headerItem->font();
        font.setBold(true);
        font.setPointSize(14);
        headerItem->setFont(font);
        headerItem->setBackground(QColor("#f0f0f0"));
        ui->list->addItem(headerItem);

        // 4. 해당 날짜에 속한 영상 파일들의 썸네일을 추가합니다.
        for (const QFileInfo& fi : filesOnDate) {
            auto *item = new QListWidgetItem();
            item->setText(fi.fileName());
            item->setToolTip(fi.absoluteFilePath());
            item->setIcon(makeThumbFor(fi.absoluteFilePath()));
            ui->list->addItem(item);
        }
    }
}

QIcon Tab2_video::makeThumbFor(const QString& filePath, QSize target)
{
    cv::VideoCapture cap(filePath.toStdString());
    if (!cap.isOpened()) {
        QPixmap px(target); px.fill(Qt::black);
        QPainter p(&px);
        p.setPen(Qt::white);
        p.drawText(px.rect(), Qt::AlignCenter, "NO PREVIEW");
        p.end();
        return QIcon(px);
    }

    cv::Mat frame;
    for (int i=0; i<3; ++i) cap.read(frame);
    if (frame.empty()) cap.read(frame);
    cap.release();

    if (frame.empty()) {
        QPixmap px(target); px.fill(Qt::black);
        return QIcon(px);
    }

    cv::Mat rgb; cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    QImage qimg(rgb.data, rgb.cols, rgb.rows, (int)rgb.step, QImage::Format_RGB888);
    QImage scaled = qimg.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return QIcon(QPixmap::fromImage(scaled));
}

void Tab2_video::playSelected()
{
    auto *item = ui->list->currentItem();
    if (!item) return;

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
    // 필요시 감지 팝업 구현 (생략 가능)
    // 여러개 뜨는 팝업 일단 주석했습니다.
    //QMessageBox::information(this, QStringLiteral("알림"), QStringLiteral("감지"));
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
