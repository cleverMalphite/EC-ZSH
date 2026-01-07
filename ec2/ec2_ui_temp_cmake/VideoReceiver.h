#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include <QDateTime>
#include <QElapsedTimer>

#include <QWidget>
#include <QTimer>
#include <future>
#include <thread>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>

#include "playerwidget.h"


//QT_BEGIN_NAMESPACE
//namespace Ui { class VideoReceiver; }
//QT_END_NAMESPACE

//udp
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include "Util/Ffmpeg_Decode.h"
#include "function.h"

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <thread>
#include <utility>
#include <mutex>

////socket struct
//typedef struct socket_info
//{
//    int serSocket;
//    int numClient;
//    sockaddr_in remoteAddr[10];
//    //int nAddrLen[10];
//    socklen_t nAddrLen[10];
//
//}Socket_Udp_Server_Info;

class VideoReceiver : public QWidget
{
    Q_OBJECT

public:
    VideoReceiver(QWidget *parent = nullptr);
    void ShowImage(cv::Mat& mat);
    
    ~VideoReceiver();

    void init();
    void decode_show();
    void setData(int videoPort);
    void endRecvVideo();
    void startRecvVideo();
    void reset();
    int getVideoPort();

public slots:
    void onRecvVideoData(QByteArray data,unsigned int dataLength,unsigned int packetIndex);

signals:
    void startSendVideo(QString remoteAddr,QString videoPort,QString commandPort,QString remoteTID);
    void sendEndCommand();
    void endVideoTrans(unsigned int TID_closeVideoTrans);

private slots:
    void on_pushButton_return_clicked();
    void udp_recv_func();
    void decode_show_func();

    void print_test();


private:
    //Ui::VideoReceiver *ui;
    bool exit_flag;
    
//    Socket_Udp_Server_Info * initUdpServer(int port);

    cv::VideoCapture video;
    QTimer *timer;
    QTimer *timer1;
    QTimer *timer2;

    int fps;
    QString videofileName;
    std::string sfileName;

    //houlc
    bool is_play = false;
    int recv_width;
    int recv_height;
    int recv_channel;
    int recv_bufferSize;

    std::mutex mtx_list;
    std::list<std::pair<char*, int>> list_pbuff;
    
    std::mutex mtx_rgblist;
    //std::list<Mat> list_matbuff;
    std::list<Mat*> list_matbuff;

    int videoListenPort;

    Ffmpeg_Decoder ffmpegobj;
//    Socket_Udp_Server_Info * mSocketUdpServerInfo;
    //default video param
    int video_cols=640;
    int video_rows=480;
    int video_chans=3;

    //houlc shwo image
    QImage image2show;


    //houlc playerWidget
    PlayerWidget *playerWidget;


    //houlc data rate statistic
    QTimer *timer_test;
    bool sta_first_get;
    QElapsedTimer sta_timer;
    qint64 sta_time_pass;
    qint64 sta_data_sum;
    qint64 sta_time_start = 0;
    double sta_data_rate;

    double sta_pic_sum;

    //houlc record
    FILE* record_fp;
    std::string record_filename = "record_recv_video.h264";

    uint16_t next_packetIndex=0;
    std::map<unsigned int,std::pair<char*,unsigned int>> packetDataAndIndexs;  //<packetIndex<packetData,dataLength>>
    int unFindPakcetCount=0;
};
#endif // MAINWINDOW_H
