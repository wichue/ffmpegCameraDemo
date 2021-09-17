#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label->setScaledContents(true);

    qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<avcodec_configuration();//打印ffmpeg配置
    qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<avcodec_version();//打印ffmpeg版本号

    pCGUsbCamera = new CGUsbCamera();
    pCGUsbCamera->isCapture(true);
//    pCGUsbCamera->SetcameraName("USB Camera");//EasyCamera，048B4，USB Camera，S500P
    pCGUsbCamera->isVideoRecord(true);
    pCGUsbCamera->SetCameraResolution(1280,720,30);

    GainCameraDevInfo_main();
    connect(pCGUsbCamera,&CGUsbCamera::sig_GetOneFrame,this,&MainWindow::slot_GetOneFrame);
    connect(ui->comboBox_cameList,&QComboBox::currentTextChanged,this,&MainWindow::slotcomboBox_cameListChanged);
}

MainWindow::~MainWindow()
{
    pCGUsbCamera->isCapture(false);
    if(pCGUsbCamera->isRunning())
    {
        pCGUsbCamera->quit();
        pCGUsbCamera->wait();
    }
    delete ui;
}


void MainWindow::printInputVideoDev()
{
    //打印视频音频设备
    AVFormatContext *pFmtCtx = avformat_alloc_context();
    AVDictionary* options = nullptr;
    av_dict_set(&options, "list_devices", "true", 0);
    AVInputFormat *iformat = av_find_input_format("dshow");
    //printf("Device Info=============\n");
    avformat_open_input(&pFmtCtx, "video=dummy", iformat, &options);
    //printf("========================\n");
}

void MainWindow::GainCameraDevInfo_main()
{
    QMap<QString,QString> tmCameraDev = pCGUsbCamera->GainCameraDevInfo();
    for(QMap<QString,QString>::iterator it=tmCameraDev.begin();it!=tmCameraDev.end();it++)
    {
        ui->comboBox_cameList->addItem(it.key());
    }

    if(tmCameraDev.size() > 0)
    {
        slotcomboBox_cameListChanged(ui->comboBox_cameList->currentText());
    }
}

void MainWindow::slot_GetOneFrame(QImage image)
{
    ui->label->setPixmap(QPixmap::fromImage(image));
}

void MainWindow::slotcomboBox_cameListChanged(QString text)
{
    QStringList list = pCGUsbCamera->GainCameraResolution(text);
    ui->comboBox_rosul->clear();
    ui->comboBox_rosul->addItems(list);
}

//开始采集
void MainWindow::on_pushButton_start_clicked()
{
#ifdef Q_OS_LINUX
//linux系统
    QString paraStr = ui->comboBox_rosul->currentText();
    if(paraStr.isEmpty())
        return;
    QStringList list = paraStr.split("x");
    pCGUsbCamera->SetCameraResolution(list.at(0).toInt(),list.at(1).toInt(),30);
#endif

#ifdef Q_OS_WIN32
    if(!ui->comboBox_rosul->currentText().isEmpty())
    {
        QString paraStr = ui->comboBox_rosul->currentText();
        QStringList list = paraStr.split("#");
        QStringList resolList = list.at(1).split("x");
        pCGUsbCamera->SetCameraResolution(resolList.at(0).toInt(),resolList.at(1).toInt(),list.at(2).toInt());
    }
#endif

    pCGUsbCamera->SetcameraName(ui->comboBox_cameList->currentText());
    pCGUsbCamera->start();
}
