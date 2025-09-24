#ifndef TAB2_VIDEO_H
#define TAB2_VIDEO_H

#include <QWidget>
#include <QPointer>
#include <QIcon>

class QListWidgetItem;
class QMediaPlayer;
class QVideoWidget;

namespace Ui { class Tab2_video; }

class Tab2_video : public QWidget
{
    Q_OBJECT
public:
    explicit Tab2_video(const QString& mediaDir = QString(), QWidget *parent = nullptr);
    explicit Tab2_video(QWidget *parent);                     // 편의 오버로드
    explicit Tab2_video(QWidget *parent, const QString& mediaDir);
    ~Tab2_video();

    void setMediaDirectory(const QString& dir);
    QString mediaDirectory() const { return m_mediaDir; }

public slots:
    void refreshGallery();
    void playSelected();
    void backToGallery();
    void onDetected();               // (선택) 감지 팝업

private:
    // UI
    Ui::Tab2_video *ui = nullptr;

    // 미디어
    QMediaPlayer  *m_player = nullptr;
    QVideoWidget  *m_video  = nullptr;
    QString        m_mediaDir;

    // 내부 유틸
    QIcon makeThumbFor(const QString& filePath, QSize target = QSize(240, 135));
    void  setupVideoOutput();    // videoHost에 QVideoWidget 주입
    void  centerAndRaise(QWidget *dlg);
};

#endif // TAB2_VIDEO_H
