#include "cgusbcamera.h"
//#include "ui_cgusbcamera.h"

int64_t CGUsbCamera::lastReadPacktTime = 0;
CGUsbCamera::CGUsbCamera(QString cameName, QThread *parent) :
    QThread(parent),
    mCameName(cameName)
{
    bVideoRecord = false;
//    GainCameraDevInfo();
//    GainAudioDevInfo();
//    GainCameraResolution("EasyCamera");

    bCapture = false;
    //分配AVFrame并将其字段设置为默认值
    videoFrame = av_frame_alloc();
    pSwsVideoFrame = av_frame_alloc();
    pSwsVideoFrame_pic = av_frame_alloc();
    Init();
}

CGUsbCamera::~CGUsbCamera()
{
    CloseInput();
    CloseOutput();
}

/*static*/ int CGUsbCamera::interrupt_cb(void *ctx)
{
    int  timeout  = 3;
    if(av_gettime() - lastReadPacktTime > timeout *1000 *1000)
    {
        return -1;
    }
    return 0;
}

int CGUsbCamera::OpenInput(string inputUrl)
{
    inputContext = avformat_alloc_context();//分配一个AVFormatContext,查找用于输入的设备
    lastReadPacktTime = av_gettime();
    inputContext->interrupt_callback.callback = interrupt_cb;
    //使用libavdevice的时候，唯一的不同在于需要首先查找用于输入的设备
#ifdef Q_OS_LINUX
    AVInputFormat *ifmt = av_find_input_format("video4linux2");
#endif

#ifdef Q_OS_WIN32
    AVInputFormat *ifmt = av_find_input_format("dshow");
#endif
     AVDictionary *format_opts =  nullptr;

     //只能设置摄像头支持的分辨率和帧率，否则打开摄像头失败，软件崩溃
    QString resolu = QString("%1x%2").arg(QString::number(mCameResolWidth)).arg(QString::number(mCameResolHeight));
     av_dict_set(&format_opts,"video_size",resolu.toLatin1().data(),0);//640x480，设置分辨率
     av_dict_set(&format_opts,"framerate",QString::number(mfps).toLatin1().data(),0);//设置帧率
     av_dict_set_int(&format_opts, "rtbufsize", 18432000  , 0);

    int ret = avformat_open_input(&inputContext, inputUrl.c_str(), ifmt,&format_opts);
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Input file open input failed\n");
        return  ret;
    }
    //读取一个媒体文件的数据包以获取流信息
    ret = avformat_find_stream_info(inputContext,nullptr);
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Find input file stream inform failed\n");
    }
    else
    {
        av_log(NULL, AV_LOG_FATAL, "Open input file  %s success\n",inputUrl.c_str());
    }
    return ret;
}


shared_ptr<AVPacket> CGUsbCamera::ReadPacketFromSource()
{
    shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p);});
    av_init_packet(packet.get());
    lastReadPacktTime = av_gettime();
    int ret = av_read_frame(inputContext, packet.get());
    if(ret >= 0)
    {
        return packet;
    }
    else
    {
        return nullptr;
    }
}

int CGUsbCamera::OpenOutput(string outUrl,AVCodecContext *encodeCodec)
{

    int ret  = avformat_alloc_output_context2(&outputContext, nullptr, "mp4", outUrl.c_str());
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open output context failed\n");
        goto Error;
    }

    ret = avio_open2(&outputContext->pb, outUrl.c_str(), AVIO_FLAG_WRITE,nullptr, nullptr);
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "open avio failed");
        goto Error;
    }
    //循环查找数据包包含的流信息，直到找到视频类型的流
    //  便将其记录下来 保存到videoStream变量中
    for(int i = 0; i < inputContext->nb_streams; i++)
    {
        if(inputContext->streams[i]->codec->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO)
        {
            continue;
        }
        AVStream * stream = avformat_new_stream(outputContext, encodeCodec->codec);
        ret = avcodec_copy_context(stream->codec, encodeCodec);
        if(ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "copy coddec context failed");
            goto Error;
        }
    }

    ret = avformat_write_header(outputContext, nullptr);
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "format write header failed");
        goto Error;
    }

    av_log(NULL, AV_LOG_FATAL, " Open output file success %s\n",outUrl.c_str());
    return ret ;
Error:
    if(outputContext)
    {
        for(int i = 0; i < outputContext->nb_streams; i++)
        {
            avcodec_close(outputContext->streams[i]->codec);
        }
        avformat_close_input(&outputContext);
    }
    return ret ;
}

void CGUsbCamera::Init()
{
    av_register_all();//初始化FFMPEG  调用了这个才能正常适用编码器和解码器
    avfilter_register_all();
    avformat_network_init();//初始化FFmpeg网络模块
    avdevice_register_all();//初始化libavdevice并注册所有输入和输出设备
    av_log_set_level(AV_LOG_ERROR);
}

void CGUsbCamera::CloseInput()
{
    if(inputContext != nullptr)
    {
        avformat_close_input(&inputContext);
    }

    if(pSwsContext)
    {
        sws_freeContext(pSwsContext);
    }
    if(pSwsContext_pic)
    {
        sws_freeContext(pSwsContext_pic);
    }
}

void CGUsbCamera::CloseOutput()
{
    if(bVideoRecord == false)
        return;//不录像
    if(outputContext != nullptr)
    {
        int ret = av_write_trailer(outputContext);
        avformat_close_input(&outputContext);
    }
}

int CGUsbCamera::WritePacket(shared_ptr<AVPacket> packet)
{
    auto inputStream = inputContext->streams[packet->stream_index];
    auto outputStream = outputContext->streams[packet->stream_index];
    packet->pts = packet->dts = packetCount * (outputContext->streams[0]->time_base.den) /
                     outputContext->streams[0]->time_base.num / 30 ;
    //cout <<"pts:"<<packet->pts<<endl;
    packetCount++;
    return av_interleaved_write_frame(outputContext, packet.get());
}

int CGUsbCamera::InitDecodeContext(AVStream *inputStream)
{
    auto codecId = inputStream->codec->codec_id;
    auto codec = avcodec_find_decoder(codecId);
    if (!codec)
    {
        return -1;
    }

    int ret = avcodec_open2(inputStream->codec, codec, NULL);//打开解码器
    return ret;

}

int CGUsbCamera::initEncoderCodec(AVStream* inputStream,AVCodecContext **encodeContext)
    {
        AVCodec *  picCodec;

        picCodec = avcodec_find_encoder(AV_CODEC_ID_H264);//软编码
        (*encodeContext) = avcodec_alloc_context3(picCodec);

        (*encodeContext)->codec_id = picCodec->id;
        (*encodeContext)->has_b_frames = 0;
        (*encodeContext)->time_base.num = inputStream->codec->time_base.num;
        (*encodeContext)->time_base.den = inputStream->codec->time_base.den;
        (*encodeContext)->pix_fmt =  *picCodec->pix_fmts;
        (*encodeContext)->width = inputStream->codec->width;
        (*encodeContext)->height =inputStream->codec->height;
        (*encodeContext)->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        int ret = avcodec_open2((*encodeContext), picCodec, nullptr);//打开解码器
        if (ret < 0)
        {
            std::cout<<"open video codec failed"<<endl;
            return  ret;
        }
            return 1;
    }

bool CGUsbCamera::Decode(AVStream* inputStream,AVPacket* packet, AVFrame *frame)
{
    int gotFrame = 0;
    //解码一帧视频数据
    auto hr = avcodec_decode_video2(inputStream->codec, frame, &gotFrame, packet);
    if (hr >= 0 && gotFrame != 0)
    {
        return true;
    }
    return false;
}


std::shared_ptr<AVPacket> CGUsbCamera::Encode(AVCodecContext *encodeContext,AVFrame * frame)
{
    int gotOutput = 0;
    std::shared_ptr<AVPacket> pkt(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p); });
    av_init_packet(pkt.get());
    pkt->data = NULL;
    pkt->size = 0;
    int ret = avcodec_encode_video2(encodeContext, pkt.get(), frame, &gotOutput);
    if (ret >= 0 && gotOutput)
    {
        return pkt;
    }
    else
    {
        return nullptr;
    }
}


int CGUsbCamera::initSwsContext(struct SwsContext** pSwsContext/*, SwsScaleContext *swsScaleContext*/)
{
    //chw，录像
    if(bVideoRecord == true)
    {
        *pSwsContext = sws_getContext(srcWidth, srcHeight, iformat,
            dstWidth, dstHeight,oformat,
            SWS_BICUBIC,
            NULL, NULL, NULL);
    }

    //chw，采集图片
    pSwsContext_pic = sws_getContext(srcWidth, srcHeight, iformat,
        dstWidth, dstHeight,AV_PIX_FMT_RGB32,
        SWS_BICUBIC,
        NULL, NULL, NULL);

    if (pSwsContext == NULL)
    {
        return -1;
    }
    return 0;
}

int CGUsbCamera::initSwsFrame(AVFrame *pSwsFrame, int iWidth, int iHeight)
{
    int numBytes = av_image_get_buffer_size(encodeContext->pix_fmt, iWidth, iHeight, 1);
    /*if(pSwpBuffer)
    {
        av_free(pSwpBuffer);
    }*/
    pSwpBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pSwsFrame->data, pSwsFrame->linesize, pSwpBuffer, encodeContext->pix_fmt, iWidth, iHeight, 1);
    pSwsFrame->width = iWidth;
    pSwsFrame->height = iHeight;
    pSwsFrame->format = encodeContext->pix_fmt;
    return 1;
}

void CGUsbCamera::run()
{
    openCameraDev();
}

/**
 * @brief 设置摄像头名称
 * @param cameName
 */
void CGUsbCamera::SetcameraName(QString cameName)
{
    mCameName = cameName;
}

/**
 * @brief 设置摄像头分辨率和帧率
 * @param width 分辨率宽
 * @param height 分辨率高
 * @param fps 帧率
 */
void CGUsbCamera::SetCameraResolution(int width, int height, int fps)
{
    mCameResolWidth = width;
    mCameResolHeight = height;
    mfps = fps;
}

/**
 * @brief CGUsbCamera::isVideoRecord
 * @param is 是否录像
 */
void CGUsbCamera::isVideoRecord(bool is)
{
    bVideoRecord = is;
}

/**
 * @brief 打开摄像头设备，开始采集
 */
void CGUsbCamera::openCameraDev()
{
    if(mCameName.isEmpty())
    {
        qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<"还未设置摄像头名称 ";
        return;
    }

#ifdef Q_OS_LINUX
    int ret = OpenInput(QString("%1").arg(mCameName).toStdString());
#endif

#ifdef Q_OS_WIN32
    int ret = OpenInput(QString("video=%1").arg(mCameName).toStdString());
#endif

    if(ret <0)
    {
        qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<"open camera failure ";
        CloseInput();
        CloseOutput();
    }

    InitDecodeContext(inputContext->streams[0]);

    ret = initEncoderCodec(inputContext->streams[0],&encodeContext);

    if(ret >= 0)
    {
        if(bVideoRecord == true)
        {
            QString dt = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
            QString filename = QCoreApplication::applicationDirPath() + "/" + dt + ".mp4";
            ret = OpenOutput(filename.toStdString(),encodeContext);//chw，录像MP4
        }
    }
    if(ret <0)
    {
        CloseInput();
        CloseOutput();
    }

    SetSrcResolution(inputContext->streams[0]->codec->width, inputContext->streams[0]->codec->height);

    SetDstResolution(encodeContext->width,encodeContext->height);
    SetFormat(inputContext->streams[0]->codec->pix_fmt, encodeContext->pix_fmt);
    initSwsContext(&pSwsContext/*, &swsScaleContext*/);
    initSwsFrame(pSwsVideoFrame,encodeContext->width, encodeContext->height);
    int64_t startTime = av_gettime();

    //用于图像采集，发送到界面
    uint8_t *out_buffer;
    int numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, encodeContext->width,encodeContext->height);
    out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
//    根据指定的图像参数和提供的图像数据缓冲区设置图像域
    avpicture_fill((AVPicture *) pSwsVideoFrame_pic, out_buffer, AV_PIX_FMT_RGB32,
            encodeContext->width, encodeContext->height);
    while(bCapture)
    {

        auto packet = ReadPacketFromSource();//使用智能指针
//        if(av_gettime() - startTime > 30 * 1000 * 1000)//只采集30s
//        {
//            break;
//        }
        if(packet && packet->stream_index == 0)
        {
            if(Decode(inputContext->streams[0],packet.get(),videoFrame))
            {
                //chw，采集图片
                //分配和返回一个SwsContext你需要它来执行使用sws_scale()的缩放/转换操作
                //在 pFrame->data 中缩放图像切片，并将得到的缩放切片放在pFrameRGB->data图像中
                sws_scale(pSwsContext_pic, (const uint8_t *const *)videoFrame->data,
                          videoFrame->linesize, 0, inputContext->streams[0]->codec->height, (uint8_t *const *)pSwsVideoFrame_pic->data, pSwsVideoFrame_pic->linesize);
                QImage tmpImg((uchar *)out_buffer,encodeContext->width,encodeContext->height,QImage::Format_RGB32);
                emit sig_GetOneFrame(tmpImg);  //发送信号

                //录像生成MP4文件
                if(bVideoRecord == true)
                {
                    sws_scale(pSwsContext, (const uint8_t *const *)videoFrame->data,
                              videoFrame->linesize, 0, inputContext->streams[0]->codec->height, (uint8_t *const *)pSwsVideoFrame->data, pSwsVideoFrame->linesize);

                    auto packetEncode = Encode(encodeContext,pSwsVideoFrame);
                    if(packetEncode)
                    {
                        ret = WritePacket(packetEncode);
                        //cout <<"ret:" << ret<<endl;
                    }
                }
            }
        }
        if(packet.get() != nullptr)
            av_packet_unref(packet.get());//释放资源
    }
    cout <<"Get Picture End "<<endl;
    av_frame_free(&videoFrame);
    avcodec_close(encodeContext);
    av_frame_free(&pSwsVideoFrame);

    CloseInput();
    CloseOutput();
}

void CGUsbCamera::isCapture(bool is)
{
    bCaptureMutex.lock();
    bCapture = is;
    bCaptureMutex.unlock();
}

/**
 * @brief 执行ffmpeg命令行
 * @param cmd 命令
 * @return 返回值
 */
QString CGUsbCamera::ffmpegCMDexec(QString cmd)
{
    //执行命令
    QProcess t_Process;//应用程序类
    t_Process.setProcessChannelMode(QProcess::MergedChannels);
    t_Process.start("cmd");//启动cmd程序,传入参数
    bool isok = t_Process.waitForStarted();
//    qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<isok;//打印启动是否成功

    QString command1 = "cd /d " + QCoreApplication::applicationDirPath() + "\r\n";
    t_Process.write(command1.toLatin1().data());//
    t_Process.write(cmd.toLatin1().data());//

    t_Process.closeWriteChannel();//关闭输入通道
    t_Process.waitForFinished();
    QString strTemp=QString::fromLocal8Bit(t_Process.readAllStandardOutput());//获取程序输出
    t_Process.close();//关闭程序

    return  strTemp;
}

/**
 * @brief 执行ffmpeg命令行,linux
 * @param cmd
 * @return
 */
QString CGUsbCamera::ffmpegCMDexecLinux(QString cmd)
{
    //执行命令
    QProcess t_Process;//应用程序类
    t_Process.setProcessChannelMode(QProcess::MergedChannels);
    t_Process.start("bash");//启动cmd程序,传入参数
    bool isok = t_Process.waitForStarted();
//    qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<isok;//打印启动是否成功

    t_Process.write(cmd.toLatin1().data());//

    t_Process.closeWriteChannel();//关闭输入通道
    t_Process.waitForFinished();
    QString strTemp=QString::fromLocal8Bit(t_Process.readAllStandardOutput());//获取程序输出
    t_Process.close();//关闭程序

    return  strTemp;
}

/**
 * @brief 获取计算机有效摄像头信息
 * @return
 */
QMap<QString, QString> CGUsbCamera::GainCameraDevInfo()
{
#ifdef Q_OS_LINUX
//linux系统
    QString ret = ffmpegCMDexecLinux("ls /dev/video*");
    mmCameraDev.clear();
    qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<ret;
    if(ret.contains("无法访问"))
        return mmCameraDev;
    QStringList list = ret.split("\n");
    for(int index=0;index<list.size();index++)
    {
        if(list.at(index).isEmpty())
            continue;
        mmCameraDev[list.at(index)] = list.at(index);
    }
    return mmCameraDev;
#endif

#ifdef Q_OS_WIN32
//windows系统
    mmCameraDev.clear();
    QString ret = ffmpegCMDexec("ffmpeg.exe -list_devices true -f dshow -i dummy\r\n");
    if(!ret.contains("Alternative name"))
    {
        qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<"检测不到音视频设备 ";
        return mmCameraDev;
    }
    QStringList list = ret.split("\"");

    //检测到视频设备
    if(list.at(0).contains("DirectShow video devices"))
    {
        int num = 0;//音频设备起始编号
        for(int index=0;index<list.size();index++)
        {
            if(list.at(index).contains("DirectShow audio devices"))
            {
                num = index;
            }
        }
        for(int index2=0;index2<(1 + (num-4)/4);index2++)
        {
            if(index2 == 0)
            {
                mmCameraDev[list.at(index2*3 + 1)] = list.at(index2*3 + 3);
            }
            else
            {
                mmCameraDev[list.at(index2*3 + 1 +1)] = list.at(index2*3 + 3 +1);
            }
        }
    }

    return mmCameraDev;
#endif
}

/**
 * @brief 获取计算机音频设备信息
 * @return
 */
QMap<QString, QString> CGUsbCamera::GainAudioDevInfo()
{
#ifdef Q_OS_LINUX
//linux系统
#endif

#ifdef Q_OS_WIN32
//windows系统

    mmAudioDev.clear();
    QString ret = ffmpegCMDexec("ffmpeg.exe -list_devices true -f dshow -i dummy\r\n");
    if(!ret.contains("Alternative name"))
    {
        qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<"检测不到音视频设备 ";
        return mmAudioDev;
    }
    QStringList list = ret.split("\"");

    int num = 0;//音频设备起始编号
    for(int index=0;index<list.size();index++)
    {
        if(list.at(index).contains("DirectShow audio devices"))
        {
            num = index;
        }
    }
//    qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<num;

    for(int index2=0;index2<(1 + (list.size() +1 -num-4)/4);index2++)
    {
        if(index2 == 0)
        {
            qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<QString(gbkFromUnicode(list.at(index2*3 + 1+num)));
            qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<list.at(index2*3 + 3+num);
            mmAudioDev[list.at(index2*3 + 1)] = list.at(index2*3 + 3);
        }
        else
        {
            qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<QString(gbkFromUnicode(list.at(index2*3 + 1 +1+num)));
            qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<list.at(index2*3 + 3 +1+num);
            mmAudioDev[list.at(index2*3 + 1 +1)] = list.at(index2*3 + 3 +1);
        }
    }

//    for(int index=0;index<list.size();index++)
//    {
//        qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<list.at(index);
//    }

    return mmAudioDev;
#endif
}

/**
 * @brief 获取摄像头分辨率
 * @param cameraName
 * @return  pixel_format#分辨率#帧率
 */
QStringList CGUsbCamera::GainCameraResolution(QString cameraName)
{
#ifdef Q_OS_LINUX
    //获取摄像头支持的分辨率
    //v4l2-ctl -d /dev/video0 --list-formats-ext//获取摄像头支持的图像格式、分辨率、帧率
    QString ret = ffmpegCMDexecLinux(QString("v4l2-ctl --list-framesizes=MJPG -d %1").arg(cameraName));
    mlCameraResolution.clear();
    QStringList list = ret.split("\n");
    for(int index=0;index<list.size();index++)
    {
        if(list.at(index).contains("Discrete"))
        {
            QStringList listCame = list.at(index).split("Discrete");
            mlCameraResolution.append(listCame.at(1).trimmed());
        }
    }

    return mlCameraResolution;
#endif
#ifdef Q_OS_WIN32
//windows系统
    mlCameraResolution.clear();
    QString cmdCommand = QString("ffmpeg.exe -list_options true -f dshow -i video=""%1""\r\n").arg(cameraName);
    QString ret = ffmpegCMDexec(cmdCommand);

    QStringList list = ret.split("\n");

    for(int index=0;index<list.size();index++)
    {
        if(list.at(index).contains("pixel_format"))
        {
            //获取关键信息[dshow @ 00000272d867e9c0]   pixel_format=yuyv422  min s=1280x720 fps=30 max s=1280x720 fps=30
            QStringList sublist = list.at(index).split(" ");
            QString pixel_format;//像素格式
            QString resolution;//分辨率
            QString fps;//帧率
            for(int index2=0;index2<sublist.size();index2++)
            {
                if(sublist.at(index2).size() < 3)
                    continue;
                if(sublist.at(index2).contains("pixel_format"))//获取像素格式
                {
                    QStringList subsublist = sublist.at(index2).split("=");
                    if(subsublist.size()== 2)
                        pixel_format = subsublist.at(1);
                }
                if(sublist.at(index2).front() == 's' && sublist.at(index2).at(1) == '=')//获取分辨率
                {
                    QStringList subsublist = sublist.at(index2).split("=");
                    if(subsublist.size()== 2)
                        resolution = subsublist.at(1);
                }
                if(sublist.at(index2).contains("fps="))//获取帧率
                {
                    QStringList subsublist = sublist.at(index2).split("=");
                    if(subsublist.size()== 2)
                        fps = subsublist.at(1);
                    fps.remove('\r');
                }
//                qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<sublist.at(index2);
            }
            mlCameraResolution.append(QString("%1#%2#%3").arg(pixel_format).arg(resolution).arg(fps));
        }
//        qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<list.at(index);
    }

    mlCameraResolution = mlCameraResolution.toSet().toList();
    for(int i=0;i<mlCameraResolution.size();i++)
    {
//        qDebug()<<"["<<__FILE__<<"]"<<__LINE__<<__FUNCTION__<<" "<<mlCameraResolution.at(i);
    }

    return mlCameraResolution;
#endif
}

/**
 * @brief 转换编码，在Qt程序里显示中文
 * @param chars
 * @return
 */
QString CGUsbCamera::gbkToUnicode(const char *chars)
{
    QTextCodec *gbk= QTextCodec::codecForName("GBK");
    QString str = gbk->toUnicode(chars);//获取的字符串可以显示中文

    return str;
}

/**
 * @brief Qt输出字符串到外部程序，在外部程序显示中文
 * @param str
 * @return
 */
QByteArray CGUsbCamera::gbkFromUnicode(QString str)
{
    QTextCodec *gbk = QTextCodec::codecForName("GBK");
    return gbk->fromUnicode(str);
}
