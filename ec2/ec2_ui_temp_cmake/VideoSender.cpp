#include "VideoSender.h"

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>

//#define DEBUG_VIDEO_SENDER_
#define TEST_VIDEO_SENDER_



VideoSender::VideoSender(QWidget *parent)
    : QWidget(parent)
{

    //init param
    fps = 10;

    //init timer 
    //ui-> label_image->setScaledContents(true);//可以使图片完全按QWidget缩放，而不保持原视频比例
    timer=new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(ReadFrame()));

    timer1=new QTimer(this);
    connect(timer1,SIGNAL(timeout()),this,SLOT(EncodeFrame()));


    //init widget
    this->resize(800, 600);
    this->setWindowTitle(QString::fromUtf8("Video Sender"));
    
    
    std::cout<<"VideoReceiver-init-5"<<std::endl;
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    this->playerWidget = new PlayerWidget(this);
    this->playerWidget->setLockLastImg(true);


    mainLayout->addWidget(playerWidget);
    
    QPushButton *playButton = new QPushButton("PLAY", this);
    connect(playButton,
            &QPushButton::clicked,
            this, [this](){
                if(this->is_play==false){
                    this->on_pushButton_play_clicked();
                    this->is_play = true;
                }
            });
    playButton->setVisible(true);

    QPushButton *stopButton = new QPushButton("STOP", this);
    connect(stopButton,
            &QPushButton::clicked,
            this, [this](){
                if(this->is_play==true){
                    this->on_pushButton_pause_clicked();
                    this->is_play = false;
                }
            });
    stopButton->setVisible(true);

    QPushButton *returnButton = new QPushButton("RETURN", this);
    connect(returnButton,
            &QPushButton::clicked,
            this, [this](){
                    this->on_pushButton_return_clicked();
            });
    returnButton->setVisible(true);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(playButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(returnButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    std::cout<<"VideoSender-init-end"<<std::endl;

    this->init();
    playerWidget->startPlay(fps);
    timer->start(1000/fps);
    this->is_play=true;
}

void VideoSender::init() {
    if(has_camera_init==false)
    {
        //cv::VideoCapture capture(0);
        fps = 30;
        video = cv::VideoCapture(0);
        //Houlc capture set
        video.set(CAP_PROP_FRAME_WIDTH, 640);//宽度
        video.set(CAP_PROP_FRAME_HEIGHT, 480);//高度
        video.set(CAP_PROP_FPS, fps);//帧率 帧/秒
        video.set(CAP_PROP_BRIGHTNESS, 100);//亮度
        video.set(CAP_PROP_CONTRAST, 40);//对比度 40
        video.set(CAP_PROP_SATURATION, 50);//饱和度 50
        video.set(CAP_PROP_HUE, 50);//色调 50
        video.set(CAP_PROP_EXPOSURE, 50);//曝光 50 获取摄像头参数

        video >> _imageTemp;
        ShowImage(_imageTemp);
        has_camera_init = true;

        std::cout<<"[INIT] init video capture"<<std::endl;
    }

}

void VideoSender::ShowImage(cv::Mat& mat)
{
    this->playerWidget->pushFrame2Show(mat);
}

VideoSender::~VideoSender()
{
    //释放摄像头资源
    video.release();

    on_pushButton_return_clicked();
    delete playerWidget;
    delete timer;
    delete timer1;
}

void VideoSender::on_pushButton_play_clicked()
{
    this->playerWidget->startPlay(fps);
    this->timer->start(1000/fps);
    this->timer1->start(1000/fps);
}

void VideoSender::on_pushButton_pause_clicked()
{
    this->playerWidget->stopPlay();
    this->timer->stop();
    this->timer1->stop();
}

void VideoSender::on_pushButton_return_clicked() {

    on_pushButton_pause_clicked();
}
