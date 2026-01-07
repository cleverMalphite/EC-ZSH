#include "Ffmpeg_Decode.h"

void Ffmpeg_Decoder::Ffmpeg_Decoder_Init()
{
    avcodec_register_all();     //注册编解码器
    av_init_packet(&avpkt);     //初始化包结构
    //m_pRGBFrame = new AVFrame[1];//RGB帧数据赋值
    m_pYUVFrame = av_frame_alloc();
    filebuf = new uint8_t[1024 * 1024];//初始化文件缓存数据区

    pCodecH264 = avcodec_find_decoder(AV_CODEC_ID_H264);     //查找h264解码器
    if (!pCodecH264)
    {
        fprintf(stderr, "h264 codec not found\n");
        exit(1);
    }
    avParserContext = av_parser_init(AV_CODEC_ID_H264);

    c = avcodec_alloc_context3(pCodecH264);//函数用于分配一个AVCodecContext并设置默认值，如果失败返回NULL，并可用av_free()进行释放

    if (pCodecH264->capabilities&AV_CODEC_CAP_TRUNCATED)
        c->flags |= AV_CODEC_FLAG_TRUNCATED;	/* we do not send complete frames */
    if (avcodec_open2(c, pCodecH264, NULL) < 0)return;
    nDataLen = 0;
    finishInitScxt = false;//将格式转换器初始化标志设为否
}

void Ffmpeg_Decoder::Ffmpeg_Decoder_Show(char *pFrame, int width, int height)
{

    Mat image = Mat(height, width, CV_8UC3, pFrame);
    imshow("解码图像", image);
    waitKey(1);
}



// 假设avcodec_context是已初始化的AVCodecContext指针
void Ffmpeg_Decoder::Ffmpeg_Decoder_Flush( ) {
    mtx_ffmpeg_c.lock();
    avcodec_flush_buffers(c);
    mtx_ffmpeg_c.unlock();
}

void Ffmpeg_Decoder::Ffmpeg_Decoder_Close()
{
    sws_freeContext(scxt);//释放格式转换器资源
    finishInitScxt = false;//将格式转换器初始化标志设为否
    delete[]filebuf;
    delete[]yuv_buff;
    delete[]rgb_buff;
    av_free(m_pYUVFrame);//释放帧资源
    avcodec_close(c);//关闭解码器
    av_free(c);
}
