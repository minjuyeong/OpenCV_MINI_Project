/********************************************************************************
** Form generated from reading UI file 'mainwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWIDGET_H
#define UI_MAINWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWidget
{
public:
    QHBoxLayout *horizontalLayout;
    QTabWidget *pTabWidget;
    QWidget *pTab1;
    QWidget *pTab2;

    void setupUi(QWidget *MainWidget)
    {
        if (MainWidget->objectName().isEmpty())
            MainWidget->setObjectName("MainWidget");
        MainWidget->resize(800, 600);
        horizontalLayout = new QHBoxLayout(MainWidget);
        horizontalLayout->setObjectName("horizontalLayout");
        pTabWidget = new QTabWidget(MainWidget);
        pTabWidget->setObjectName("pTabWidget");
        pTab1 = new QWidget();
        pTab1->setObjectName("pTab1");
        pTabWidget->addTab(pTab1, QString());
        pTab2 = new QWidget();
        pTab2->setObjectName("pTab2");
        pTabWidget->addTab(pTab2, QString());

        horizontalLayout->addWidget(pTabWidget);


        retranslateUi(MainWidget);

        QMetaObject::connectSlotsByName(MainWidget);
    } // setupUi

    void retranslateUi(QWidget *MainWidget)
    {
        MainWidget->setWindowTitle(QCoreApplication::translate("MainWidget", "MainWidget", nullptr));
        pTabWidget->setTabText(pTabWidget->indexOf(pTab1), QCoreApplication::translate("MainWidget", "Tab 1", nullptr));
        pTabWidget->setTabText(pTabWidget->indexOf(pTab2), QCoreApplication::translate("MainWidget", "Tab 2", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWidget: public Ui_MainWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWIDGET_H
