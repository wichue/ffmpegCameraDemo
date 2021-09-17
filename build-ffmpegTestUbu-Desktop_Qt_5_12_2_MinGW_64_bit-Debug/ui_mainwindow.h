/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QLabel *label;
    QPushButton *pushButton_start;
    QLabel *label_3;
    QLabel *label_2;
    QComboBox *comboBox_rosul;
    QComboBox *comboBox_cameList;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(747, 590);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        label = new QLabel(centralWidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(20, 60, 1311, 651));
        pushButton_start = new QPushButton(centralWidget);
        pushButton_start->setObjectName(QString::fromUtf8("pushButton_start"));
        pushButton_start->setGeometry(QRect(640, 10, 93, 28));
        label_3 = new QLabel(centralWidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(260, 10, 141, 21));
        label_2 = new QLabel(centralWidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(10, 10, 91, 21));
        comboBox_rosul = new QComboBox(centralWidget);
        comboBox_rosul->setObjectName(QString::fromUtf8("comboBox_rosul"));
        comboBox_rosul->setGeometry(QRect(400, 10, 201, 22));
        comboBox_cameList = new QComboBox(centralWidget);
        comboBox_cameList->setObjectName(QString::fromUtf8("comboBox_cameList"));
        comboBox_cameList->setGeometry(QRect(120, 10, 121, 22));
        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 747, 28));
        MainWindow->setMenuBar(menuBar);
        mainToolBar = new QToolBar(MainWindow);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", nullptr));
        label->setText(QApplication::translate("MainWindow", "TextLabel", nullptr));
        pushButton_start->setText(QApplication::translate("MainWindow", "\345\274\200\345\247\213\351\207\207\351\233\206", nullptr));
        label_3->setText(QApplication::translate("MainWindow", "\346\224\257\346\214\201\347\232\204\345\210\206\350\276\250\347\216\207\345\222\214\345\270\247\347\216\207", nullptr));
        label_2->setText(QApplication::translate("MainWindow", "\351\200\211\346\213\251\346\221\204\345\203\217\345\244\264", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
