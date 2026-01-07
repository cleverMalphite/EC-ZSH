#include "VideoReceiver.h"
//#include "ui_VideoReceiver.h"

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <iostream>
#include <time.h>

// decode
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include "opencv2/opencv.hpp"


#include <list>
#include <thread>
#include <utility>
#include <mutex>

using namespace cv;

//#define DEBUG_VIDEO_RECEIVER_
//#define TEST_VIDEO_RECEIVER_

long consumptiontime;
long playframecount = 0;


VideoReceiver::VideoReceiver(QWidget *parent)
    : QWidget(parent)
    //, ui(new Ui::VideoReceiver)
{

    /*udp decode etc. init*/
#ifdef DEBUG_VIDEO_RECEIVER_
    std::cout<<"VideoReceiver-init-1"<<std::endl;
#endif
    exit_flag=false;
//    this->setWindowFlags(this->windowFlags()&~Qt::WindowCloseButtonHint);  //隐藏窗口的叉号

    //init param
#ifdef DEBUG_VIDEO_RECEIVER_
    std::cout<<"VideoReceiver-init-2"<<std::endl;
#endif
    int info[3];
    int flag = 0;
    int ret = 0;
    recv_width = video_cols;
    recv_height = video_rows;
    recv_channel = video_chans;
    recv_bufferSize = recv_width * recv_height * recv_channel;
    flag = 1;

    fps = 30;


}

void VideoReceiver::init(){

    /*qt ui ready*/

    //start readFrame, start playerWidget
    //this->timer->start(1000/fps);
    this->playerWidget->startPlay(fps);
    this->timer1->start(1);
    this->timer2->start(1);

    this->timer_test->start(1000);

    this->is_play = true;

    //rate statistic
    sta_first_get = false;
    sta_time_pass = 0;

    sta_data_sum = 0;
    sta_data_rate = 0;

    sta_pic_sum = 0;


    //set PlayerWidget
    //setCentralWidget(this);
    cv::Mat imageTemp = cv::Mat::zeros(recv_height, recv_width, CV_8UC3);
    this->ShowImage(imageTemp);

    printf("[init] end\n");

    //houlc record
//    record_fp = fopen(record_filename.c_str(), "wb");
}

void VideoReceiver::reset() {
    next_packetIndex=0;
    unFindPakcetCount=0;
    mtx_list.lock();
    packetDataAndIndexs.clear();
    mtx_list.unlock();
}


void VideoReceiver::ShowImage(cv::Mat& mat)
{
#ifdef DEBUG_VIDEO_RECEIVER_
    printf("show Image...\n");
#endif

    //statistic pic sum
    qint64 mat_rows = mat.rows;
    qint64 mat_cols = mat.cols;
    qint64 mat_chan = mat.channels();
    qint64 mat_elemSize = mat.elemSize();//1 for CV_8U, 4 for CV_32F
    double mat_size_B = (double)mat_rows * mat_cols * mat_chan * mat_elemSize;
    double mat_size_Mb =  mat_size_B * 8 / 1000 / 1000;
    sta_pic_sum  = sta_pic_sum + mat_size_Mb;

    this->playerWidget->pushFrame2Show(mat);

}



VideoReceiver::~VideoReceiver()
{
    on_pushButton_return_clicked();
    packetDataAndIndexs.clear();
    delete playerWidget;
    exit_flag=true;
    //delete timer;
    delete timer1;
    delete timer2;

    delete timer_test;
//    delete timer3;
    //delete ui;
    //close decoder
    ffmpegobj.Ffmpeg_Decoder_Close();

//    fclose(record_fp);
}

void VideoReceiver::on_pushButton_return_clicked() {
    emit this->sendEndCommand();
    endRecvVideo();
}

//void startSendVideo(QString remoteAddr);
//void sendEndCommand();
//void endVideoTrans();

void VideoReceiver::startRecvVideo() {
    if(this->is_play==false){
        is_play = true;
        //timer->start(1000/fps);   //show frame
        timer1->start(1);         //decode frame
        timer2->start(1);         //udp recv
        this->playerWidget->startPlay(fps);

        timer_test->start(1000);
    }
}

void VideoReceiver::endRecvVideo() {
    if(is_play==true){
        is_play = false;

        //timer->stop();
        timer1->stop(); timer2->stop();
        this->playerWidget->stopPlay();

        timer_test->stop();

        mtx_list.lock();
        list_pbuff.clear();
        packetDataAndIndexs.clear();
        mtx_list.unlock();

        mtx_rgblist.lock();
        for(int i = 0; i < list_matbuff.size(); i++){
            delete list_matbuff.front();
            list_matbuff.front() = nullptr;
            list_matbuff.pop_front();
        }
        mtx_rgblist.unlock();
    }
//    this->setVisible(false);
}

void VideoReceiver::print_test()
{
#ifdef TEST_VIDEO_RECEIVER_
    sta_time_pass = sta_timer.elapsed();
    //qint64 sta_time_curr = QDateTime::currentMSecsSinceEpoch();
    //sta_time_pass = sta_time_curr  - sta_time_start;
    if(sta_time_pass == 0) return ;

    double sta_time_pass_dw = sta_time_pass;
    double sta_time_pass_s_dw = sta_time_pass_dw / 1000;
    double sta_data_sum_dw = sta_data_sum;

    double sta_data_sum_b_dw = sta_data_sum_dw * 8;
    double sta_data_sum_kb_dw = sta_data_sum_b_dw / 1000;
    double sta_data_sum_Mb_dw = sta_data_sum_kb_dw / 1000;

    double sta_data_rate_Bpms = sta_data_sum_dw / sta_time_pass_dw ;
    double sta_data_rate_bps = sta_data_sum_b_dw / sta_time_pass_s_dw;
    double sta_data_rate_kbps = sta_data_sum_kb_dw / sta_time_pass_s_dw;

    sta_data_rate = sta_data_sum_Mb_dw / sta_time_pass_s_dw ;

    double sta_pic_rate_Mbps = sta_pic_sum / sta_time_pass_s_dw;

    printf("/============/\n");
    // 获取当前时间
    QDateTime currentTime = QDateTime::currentDateTime();
    //QString currentDateTimeString = currentTime.toString();// 使用Qt默认的短日期和时间格式（例如："2023-03-17 15:43:21"）
    QString currentDateTimeString = currentTime.toString("yyyy-MM-dd HH:mm:ss");
    std::cout<<"[TEST] TEST TIME : " <<  currentDateTimeString.toStdString() <<std::endl;
    std::cout<<"[TEST] RECV PASS I : "<< sta_time_pass <<" ms" <<std::endl;
    printf("[TEST] RECV PASS:%lf ms\n", sta_time_pass_dw);
    std::cout<<"[TEST] RECV PASS dw : "<< sta_time_pass_s_dw <<" s" <<std::endl;

    printf("/----------/\n");
    printf("[TEST] RECV SUM:%lf B\n", sta_data_sum_dw);
    printf("[TEST] RECV SUM:%lf b\n", sta_data_sum_b_dw);
    printf("[TEST] RECV SUM:%lf kb\n", sta_data_sum_kb_dw);
    printf("[TEST] RECV SUM:%lf Mb\n", sta_data_sum_Mb_dw);



    printf("[TEST] DATA RATE:%lf Bpms\n", sta_data_rate_Bpms);
    printf("[TEST] DATA RATE:%lf bps\n", sta_data_rate_bps);
    printf("[TEST] DATA RATE:%lf kbps\n", sta_data_rate_kbps);
    printf("[TEST] DATA RATE:%lf Mbps\n", sta_data_rate);

    printf("[TEST] VIDEO TRNAS SUM:%lf Mb\n", sta_pic_sum);
    printf("[TEST] VIDEO TRNAS RATE:%lf Mbps\n", sta_pic_rate_Mbps);
#endif
}

void VideoReceiver::onRecvVideoData(QByteArray data, unsigned int dataLength,unsigned int packetIndex)
{
//    cout<<"[udp_recv_func] timer2 is excuting"<<endl;
    //while(1) {
    if(!is_play)
    {

        printf("[WARN] recv_func::is_play=false\n");

        return ;
        //continue;
    }

//    fwrite(reinterpret_cast<unsigned char*>(data.data()), dataLength, 1, record_fp);

//    unsigned char* pdata=new unsigned char [dataLength];
//    mempcpy(pdata,data.constData(),dataLength);
//    fwrite(pdata, dataLength, 1, record_fp);
//    delete[] pdata;

    //usleep(1);
    /*char * revBuffer = (char *)malloc(sizeof(char) * recv_bufferSize);
    size_t ret = recvfrom(mSocketUdpServerInfo->serSocket, revBuffer, recv_bufferSize, 0,
                          (sockaddr *)&mSocketUdpServerInfo->remoteAddr[0], &mSocketUdpServerInfo->nAddrLen[0]);*/
    char* revBuffer=data.data();
    unsigned int ret=dataLength;
    if(ret==0){
        cout<<"[udp_recv_func] recvfrom=0"<<endl;
    }

    //statistic rate
    if(ret>0 && sta_first_get==false)
    {
#ifdef TEST_VIDEO_RECEIVER_
        printf("<>[DEBUG] sta timer start\n");
#endif
        sta_first_get=true;
        sta_timer.start();
    }

    if(ret>0){
        sta_data_sum = sta_data_sum + ret;

        if(sta_time_start == 0){
            sta_time_start =QDateTime::currentMSecsSinceEpoch();
#ifdef TEST_VIDEO_RECEIVER_
            printf("<>[DEBUG] sta timer start||%ld ms\n", sta_time_start);
#endif
        }
    }


#ifdef DEBUG_VIDEO_RECEIVER_
    printf("[OK] recv ret = %d \n", ret);
#endif


    char * pbuf = (char *)malloc(sizeof(char) * ret + 1);
    memset(pbuf, 0, ret + 1);
    memcpy(pbuf, revBuffer, ret);



//    free(revBuffer);
    revBuffer = nullptr;
    std::cout<<"recv packetIndex="<<packetIndex<<endl;

    mtx_list.lock();
    packetDataAndIndexs[packetIndex]=std::pair<char*,unsigned int>(pbuf,ret);
//    list_pbuff.push_back(std::pair<char*, int>(pbuf, ret));
    mtx_list.unlock();
    //}
}

/* end udp */

//thread func
//void decode_show_func(Ffmpeg_Decoder& ffmpegobj)
void VideoReceiver::decode_show_func()
{
//    cout<<"[decode_show_func] timer1 is excuting"<<endl;
    //while(true){
        //usleep(1);
        if(packetDataAndIndexs.empty()) {
            //usleep(5);
#ifdef DEBUG_VIDEO_RECEIVER_
            printf("[WARN] decode_func::lis_pbuff is empty!\n");
#endif
            return ;
            //continue;
        }
//Houlc
        time_t t;
        time(&t);
#ifdef DEBUG_VIDEO_RECEIVER_
        printf("[FUNCTION] decode_show_func()\n");
        printf("[TIME] %s\n", ctime(&t));
        printf("[QSIZE] list_pbuff size: %d\n", list_pbuff.size());
#endif


        mtx_list.lock();

        /*char* pbuff = list_pbuff.front().first;
        int ilen = list_pbuff.front().second;*/

        //找到正确序号的数据
        if(packetDataAndIndexs.find(next_packetIndex)==packetDataAndIndexs.end()){
            if(unFindPakcetCount<3){
                unFindPakcetCount+=1;
                mtx_list.unlock();
                return;
            }else{
                unFindPakcetCount=0;
                next_packetIndex+=1;
                return;
            }
        }

        unFindPakcetCount=0;
        char* pbuff=packetDataAndIndexs.find(next_packetIndex)->second.first;
        int ilen=packetDataAndIndexs.find(next_packetIndex)->second.second;

        memcpy(ffmpegobj.filebuf, pbuff, ilen);
        ffmpegobj.nDataLen = ilen;

        free(pbuff);
        pbuff = nullptr;

        packetDataAndIndexs.erase(next_packetIndex);
        next_packetIndex+=1;

//        list_pbuff.pop_front();

        mtx_list.unlock();

        //must memcpy(ffmpegobj), and set it's len before
#ifdef DEBUG_VIDEO_RECEIVER_
        printf("[OK] decode_func::decode_show() ... wait");
#endif
        //decode_show(ffmpegobj);
        decode_show();
#ifdef DEBUG_VIDEO_RECEIVER_
        printf(" ... ok!\n");
#endif
    //}

}

/* end decode */

//Ffmpeg_Decoder ffmpegobj;
//decode func
//void decode_show(Ffmpeg_Decoder &ffmpegobj)
void VideoReceiver::decode_show()
{
   if (ffmpegobj.nDataLen <= 0)
   {
       //houlc
#ifdef DEBUG_VIDEO_RECEIVER_
       printf("[Error]ffmpegobj.nDataLen <=0 \n");
#endif
    //break;
       return ;
   }
   else
   {
    ffmpegobj.haveread = 0;
    while (ffmpegobj.nDataLen > 0)
    {
        //usleep(1);
#ifdef DEBUG_VIDEO_RECEIVER_
        printf("[houlc-debug] av_parser_parse2..start..\n");
#endif

        ffmpegobj.mtx_ffmpeg_c.lock();
        int nLength = av_parser_parse2(ffmpegobj.avParserContext, ffmpegobj.c, &ffmpegobj.yuv_buff,
            &ffmpegobj.nOutSize, ffmpegobj.filebuf + ffmpegobj.haveread, ffmpegobj.nDataLen, 0, 0, 0);//查找帧头
        ffmpegobj.mtx_ffmpeg_c.unlock();

#ifdef DEBUG_VIDEO_RECEIVER_
            printf("[houlc-debug] av_parser_parse2..ok..ret=%d\n", ffmpegobj.nOutSize);
#endif
        ffmpegobj.nDataLen -= nLength;//查找过后指针移位标志
        ffmpegobj.haveread += nLength;
        if (ffmpegobj.nOutSize <= 0)
        {

            break;
        }
        ffmpegobj.avpkt.size = ffmpegobj.nOutSize;//将帧数据放进包中
        ffmpegobj.avpkt.data = ffmpegobj.yuv_buff;
        playframecount++;

        //usleep(1);
#ifdef DEBUG_VIDEO_RECEIVER_
        printf("[houlc-debug] avcodec_decode_video2..start..\n");
#endif

        ffmpegobj.mtx_ffmpeg_c.lock();
        ffmpegobj.decodelen = avcodec_decode_video2(ffmpegobj.c, ffmpegobj.m_pYUVFrame, &ffmpegobj.piclen, &ffmpegobj.avpkt);//解码
        ffmpegobj.mtx_ffmpeg_c.unlock();

#ifdef DEBUG_VIDEO_RECEIVER_
        printf("[houlc-debug] avcodec_decode_video2..ret:%d.\n", ffmpegobj.decodelen);
#endif

#ifdef DEBUG_VIDEO_RECEIVER_
        printf("[houlc-debug] AVERROR_BUG=%ld\n", AVERROR_BUG);
#endif
           if(ffmpegobj.piclen<0)
        {
#ifdef DEBUG_VIDEO_RECEIVER_
            printf("\n[houlc]decode len < 0!!!!\n");
#endif
            break;
        }
        if (ffmpegobj.piclen)
        {
            if (ffmpegobj.finishInitScxt == false)//初始化格式转换函数
            {
                ffmpegobj.finishInitScxt = true;

                ffmpegobj.mtx_ffmpeg_c.lock();
                ffmpegobj.scxt = sws_getContext(ffmpegobj.c->width, ffmpegobj.c->height, ffmpegobj.c->pix_fmt,
                    ffmpegobj.c->width, ffmpegobj.c->height, AV_PIX_FMT_BGR24, SWS_POINT, NULL, NULL, NULL);
                ffmpegobj.mtx_ffmpeg_c.unlock();

                int width = ffmpegobj.c->width;
                int height = ffmpegobj.c->height;

                try {
                    ffmpegobj.yuv_buff = new uint8_t[width * height * 2];//初始化YUV图像数据区
                    ffmpegobj.rgb_buff = new uint8_t[width * height * 3];//初始化RGB图像帧数据区
                }
                catch(std::bad_alloc& ex){
#ifdef DEBUG_VIDEO_RECEIVER_
                    std::cout<<"[MemWrong]decode_show() ffmpegobj.yuv_buff/rgb_buff new failed: " << ex.what() <<std::endl;
#endif
                }

                ffmpegobj.data[0] = ffmpegobj.rgb_buff;
                ffmpegobj.linesize[0] = width * 3;
            }
            if (ffmpegobj.scxt != NULL)
            {
                //usleep(1);
                //YUV转rgb
                ffmpegobj.mtx_ffmpeg_c.lock();
                sws_scale(ffmpegobj.scxt, ffmpegobj.m_pYUVFrame->data, ffmpegobj.m_pYUVFrame->linesize, 0,
                    ffmpegobj.c->height, ffmpegobj.data, ffmpegobj.linesize);
                ffmpegobj.mtx_ffmpeg_c.unlock();
                //usleep(1);
                //ffmpegobj.Ffmpeg_Decoder_Show((char*)ffmpegobj.rgb_buff, ffmpegobj.c->width, ffmpegobj.c->height);//解码图像显示

                Mat *rgbmat = nullptr;

                try {
                    //Mat rgbmat = Mat(ffmpegobj.c->height, ffmpegobj.c->width, CV_8UC3, (char*)ffmpegobj.rgb_buff);
                    rgbmat = new Mat(ffmpegobj.c->height, ffmpegobj.c->width, CV_8UC3,
                                          (char *) ffmpegobj.rgb_buff);
                }
                catch(std::bad_alloc& ex){
#ifdef DEBUG_VIDEO_RECEIVER_
                    std::cout<<"[MemWrong]decode_show() new Mat push into list_matbuff // new failed: " << ex.what() <<std::endl;
#endif
                }

                //mtx_rgblist.lock();
                //list_matbuff.push_back(rgbmat);
                //mtx_rgblist.unlock();
                this->ShowImage(*rgbmat);
#ifdef DEBUG_VIDEO_RECEIVER_
                std::cout<<"----------------delete rgb mat -----------------------------------"  <<std::endl;
#endif
                delete rgbmat;
                rgbmat = nullptr;
            }
        }
    }
   }
}

int VideoReceiver::getVideoPort() {
    return videoListenPort;
}
