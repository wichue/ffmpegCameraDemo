#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include"cgusbcamera.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void printInputVideoDev();//打印输入设备
    void GainCameraDevInfo_main();//获取摄像头列表

private:
    Ui::MainWindow *ui;
    CGUsbCamera *pCGUsbCamera;

public slots:
    void slot_GetOneFrame(QImage image);
    void slotcomboBox_cameListChanged(QString text);
private slots:
    void on_pushButton_start_clicked();
};

#endif // MAINWINDOW_H
