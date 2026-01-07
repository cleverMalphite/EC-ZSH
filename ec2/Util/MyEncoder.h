#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include<libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}

#include <mutex>

class MyEncoder
{
public:
	MyEncoder();
public:
	AVFrame *m_pRGBFrame;   //֡����  
	AVFrame *m_pYUVFrame;   //֡����  
	AVCodec *pCodecH264;    //������  
	AVCodecContext *c;      //���������ݽṹ����  
	uint8_t *yuv_buff;      //yuvͼ��������  
	uint8_t *rgb_buff;      //rgbͼ��������  
	SwsContext *scxt;       //ͼ���ʽת������  
	uint8_t *outbuf;        //���������Ƶ���ݻ���  
	int outbuf_size;        //�����������ȥ��С  
	int nDataLen;           //rgbͼ������������  
	int width;              //�����Ƶ���  
	int height;             //�����Ƶ�߶�  
	AVPacket pkt;            //���ݰ��ṹ��
public:
	void Ffmpeg_Encoder_Init();//��ʼ��  
	void Ffmpeg_Encoder_Setpara(AVCodecID mycodeid, int vwidth, int vheight);//���ò���,��һ������Ϊ������,�ڶ�������Ϊѹ����������Ƶ�Ŀ�ȣ���������Ƶ��Ϊ��߶�  
	void Ffmpeg_Encoder_Encode(FILE *file, uint8_t *data);//���벢д�����ݵ��ļ� 
	void Ffmpeg_Encoder_Encode_New(uint8_t *data, AVPacket *pkt);
	void Ffmpeg_Encoder_Close();//�ر� 

    std::mutex mtx_encoder_c;
};
