#ifndef CGUSBCAMERA_H
#define CGUSBCAMERA_H

#include <QWidget>
#include <string>
#include <memory>
#include <QThread>
#include <iostream>
#include <iostream>
#include <QDebug>
#include <QMutex>
#include <QProcess>
#include <QTextCodec>
#include <QDateTime>
#include <QCoreApplication>

extern "C"
{
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libavutil/fifo.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
//    #include "libavfilter/avfiltergraph.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavdevice/avdevice.h"
}

using namespace std;

namespace Ui {
class CGUsbCamera;
}

class CGUsbCamera : public QThread
{
    Q_OBJECT

public:
    explicit CGUsbCamera(QString cameName="",QThread *parent = nullptr);
    ~CGUsbCamera();

    void SetSrcResolution(int width, int height)
    {
        srcWidth = width;
        srcHeight = height;
    }
    void SetDstResolution(int width, int height)
    {
        dstWidth = width;
        dstHeight = height;
    }
    void SetFormat(AVPixelFormat iformat, AVPixelFormat oformat)
    {
        this->iformat = iformat;
        this->oformat = oformat;
    }
public:
    int srcWidth;
    int srcHeight;
    int dstWidth;
    int dstHeight;
    AVPixelFormat iformat;
    AVPixelFormat oformat;

public:
    static int interrupt_cb(void *ctx);
    int OpenInput(string inputUrl);
    shared_ptr<AVPacket> ReadPacketFromSource();
    int OpenOutput(string outUrl,AVCodecContext *encodeCodec);
    void Init();
    void CloseInput();
    void CloseOutput();
    int WritePacket(shared_ptr<AVPacket> packet);
    int InitDecodeContext(AVStream *inputStream);
    int initEncoderCodec(AVStream* inputStream,AVCodecContext **encodeContext);
    bool Decode(AVStream* inputStream,AVPacket* packet, AVFrame *frame);
    std::shared_ptr<AVPacket> Encode(AVCodecContext *encodeContext,AVFrame * frame);
    int initSwsFrame(AVFrame *pSwsFrame, int iWidth, int iHeight);
    int initSwsContext(struct SwsContext** pSwsContext/*, SwsScaleContext *swsScaleContext*/);

    AVFormatContext *inputContext = nullptr;//FFMPEG所有的操作都要通过这个AVFormatContext来进行
    AVCodecContext *encodeContext = nullptr;
    AVFormatContext * outputContext;
    static int64_t lastReadPacktTime ;
    int64_t packetCount = 0;
    struct SwsContext* pSwsContext = nullptr;
    struct SwsContext* pSwsContext_pic = nullptr;
    uint8_t * pSwpBuffer = nullptr;

    AVFrame *videoFrame;
    AVFrame *pSwsVideoFrame;
    AVFrame *pSwsVideoFrame_pic;

    //chw
protected:
    void run();
public:
    void SetcameraName(QString cameName);//设置摄像头名称
    void SetCameraResolution(int width, int height, int fps);//设置摄像头分辨率和帧率，注意摄像头必须支持该分辨率和帧率
    void isVideoRecord(bool is);//是否录像
    void openCameraDev();//打开摄像头设备，开始采集
    void isCapture(bool is);
    QString ffmpegCMDexec(QString cmd);//执行ffmpeg命令行,win
    QString ffmpegCMDexecLinux(QString cmd);//执行ffmpeg命令行,linux
    QMap<QString,QString> GainCameraDevInfo();//获取计算机有效摄像头信息
    QMap<QString,QString> GainAudioDevInfo();//获取计算机音频设备信息
    QStringList GainCameraResolution(QString cameraName);//获取摄像头分辨率
    QMap<QString,QString> mmCameraDev;//计算机有效摄像头信息，<摄像头名,摄像头描述符>
    QMap<QString,QString> mmAudioDev;//计算机有效音频信息，<音频名,音频描述符>
    QStringList mlCameraResolution;//计算机分辨率帧率，pixel_format#分辨率#帧率
    bool bCapture;//是否采集
    bool bVideoRecord;//是否录像
    QMutex bCaptureMutex;
    QString mCameName;//摄像头名称
    int mCameResolWidth;//当前使用分辨率宽
    int mCameResolHeight;//当前使用分辨率高
    int mfps;//当前使用帧率

    QString gbkToUnicode(const char* chars);//转换编码，在Qt程序里显示中文
    QByteArray gbkFromUnicode(QString str);//Qt输出字符串到外部程序，在外部程序显示中文
signals:
    void sig_GetOneFrame(QImage);
};

#endif // CGUSBCAMERA_H
