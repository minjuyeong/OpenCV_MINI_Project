/********************************************************************************
** Form generated from reading UI file 'tab2_video.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TAB2_VIDEO_H
#define UI_TAB2_VIDEO_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Tab2_video
{
public:
    QVBoxLayout *rootLayout;
    QWidget *topBar;
    QHBoxLayout *topBarLayout;
    QLabel *title;
    QSpacerItem *spacerTop;
    QStackedWidget *stack;
    QWidget *galleryPage;
    QVBoxLayout *galleryLayout;
    QWidget *galleryCtrl;
    QHBoxLayout *galleryCtrlLayout;
    QPushButton *btnRefresh;
    QPushButton *btnPlay;
    QSpacerItem *spacerGallery;
    QListWidget *list;
    QWidget *playerPage;
    QVBoxLayout *playerLayout;
    QWidget *playerCtrl;
    QHBoxLayout *playerCtrlLayout;
    QPushButton *btnBack;
    QLabel *nowPlaying;
    QWidget *videoHost;
    QVBoxLayout *videoHostLayout;

    void setupUi(QWidget *Tab2_video)
    {
        if (Tab2_video->objectName().isEmpty())
            Tab2_video->setObjectName("Tab2_video");
        Tab2_video->resize(960, 640);
        rootLayout = new QVBoxLayout(Tab2_video);
        rootLayout->setObjectName("rootLayout");
        topBar = new QWidget(Tab2_video);
        topBar->setObjectName("topBar");
        topBarLayout = new QHBoxLayout(topBar);
        topBarLayout->setObjectName("topBarLayout");
        topBarLayout->setContentsMargins(0, 0, 0, 0);
        title = new QLabel(topBar);
        title->setObjectName("title");
        title->setStyleSheet(QString::fromUtf8("font-weight:600;"));

        topBarLayout->addWidget(title);

        spacerTop = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        topBarLayout->addItem(spacerTop);


        rootLayout->addWidget(topBar);

        stack = new QStackedWidget(Tab2_video);
        stack->setObjectName("stack");
        galleryPage = new QWidget();
        galleryPage->setObjectName("galleryPage");
        galleryLayout = new QVBoxLayout(galleryPage);
        galleryLayout->setObjectName("galleryLayout");
        galleryCtrl = new QWidget(galleryPage);
        galleryCtrl->setObjectName("galleryCtrl");
        galleryCtrlLayout = new QHBoxLayout(galleryCtrl);
        galleryCtrlLayout->setObjectName("galleryCtrlLayout");
        galleryCtrlLayout->setContentsMargins(0, 0, 0, 0);
        btnRefresh = new QPushButton(galleryCtrl);
        btnRefresh->setObjectName("btnRefresh");

        galleryCtrlLayout->addWidget(btnRefresh);

        btnPlay = new QPushButton(galleryCtrl);
        btnPlay->setObjectName("btnPlay");

        galleryCtrlLayout->addWidget(btnPlay);

        spacerGallery = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        galleryCtrlLayout->addItem(spacerGallery);


        galleryLayout->addWidget(galleryCtrl);

        list = new QListWidget(galleryPage);
        list->setObjectName("list");
        list->setViewMode(QListView::IconMode);
        list->setMovement(QListView::Static);
        list->setResizeMode(QListView::Adjust);
        list->setIconSize(QSize(240, 135));
        list->setGridSize(QSize(260, 180));
        list->setSpacing(8);
        list->setUniformItemSizes(false);
        list->setWordWrap(true);

        galleryLayout->addWidget(list);

        stack->addWidget(galleryPage);
        playerPage = new QWidget();
        playerPage->setObjectName("playerPage");
        playerLayout = new QVBoxLayout(playerPage);
        playerLayout->setObjectName("playerLayout");
        playerCtrl = new QWidget(playerPage);
        playerCtrl->setObjectName("playerCtrl");
        playerCtrlLayout = new QHBoxLayout(playerCtrl);
        playerCtrlLayout->setObjectName("playerCtrlLayout");
        playerCtrlLayout->setContentsMargins(0, 0, 0, 0);
        btnBack = new QPushButton(playerCtrl);
        btnBack->setObjectName("btnBack");

        playerCtrlLayout->addWidget(btnBack);

        nowPlaying = new QLabel(playerCtrl);
        nowPlaying->setObjectName("nowPlaying");
        nowPlaying->setStyleSheet(QString::fromUtf8("color:#555;"));

        playerCtrlLayout->addWidget(nowPlaying);


        playerLayout->addWidget(playerCtrl);

        videoHost = new QWidget(playerPage);
        videoHost->setObjectName("videoHost");
        videoHost->setMinimumSize(QSize(640, 360));
        videoHostLayout = new QVBoxLayout(videoHost);
        videoHostLayout->setObjectName("videoHostLayout");
        videoHostLayout->setContentsMargins(0, 0, 0, 0);

        playerLayout->addWidget(videoHost);

        stack->addWidget(playerPage);

        rootLayout->addWidget(stack);


        retranslateUi(Tab2_video);

        stack->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(Tab2_video);
    } // setupUi

    void retranslateUi(QWidget *Tab2_video)
    {
        Tab2_video->setWindowTitle(QCoreApplication::translate("Tab2_video", "\354\230\201\354\203\201", nullptr));
        title->setText(QCoreApplication::translate("Tab2_video", "\354\230\201\354\203\201 \352\260\244\353\237\254\353\246\254", nullptr));
        btnRefresh->setText(QCoreApplication::translate("Tab2_video", "\354\203\210\353\241\234\352\263\240\354\271\250", nullptr));
        btnPlay->setText(QCoreApplication::translate("Tab2_video", "\354\236\254\354\203\235", nullptr));
        btnBack->setText(QCoreApplication::translate("Tab2_video", "\342\206\220 \352\260\244\353\237\254\353\246\254", nullptr));
        nowPlaying->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class Tab2_video: public Ui_Tab2_video {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TAB2_VIDEO_H
