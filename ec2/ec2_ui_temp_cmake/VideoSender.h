#ifndef VIDEOSENDER_H
#define VIDEOSENDER_H

#include <iostream>
#include <string>
#include <QWidget>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>

#include "playerwidget.h"

//udp info struct
#include <QResizeEvent>
#include <stdio.h>
#include <list>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include "Util/MyEncoder.h"
#include "function.h"

using namespace cv;

class VideoSender : public QWidget
{
    Q_OBJECT

public:
    VideoSender(QWidget *parent = nullptr);
    void ShowImage(cv::Mat& mat);
    ~VideoSender();

    void init();

private slots:
    void on_pushButton_play_clicked();

    void on_pushButton_pause_clicked();

    void on_pushButton_return_clicked();
private:
    cv::VideoCapture video;
    QTimer *timer;
    QTimer *timer1;
////    QTimer *timer_onlyShow;
//    //QTimer *timer2;
    int fps;
    bool is_play;

    cv::Mat _imageTemp;

    bool has_camera_init=false;

    PlayerWidget *playerWidget;

};
#endif // VideoSender_H
