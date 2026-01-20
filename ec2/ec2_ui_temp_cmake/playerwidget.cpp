#include <iostream>
#include "playerwidget.h" 
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>

//#define DEBUG_PLAYER_WIDGET_

PlayerWidget::PlayerWidget(QWidget *parent)
    : QWidget(parent)
{
    this->len_buf = 2;
    this->fps = 10;

    // 设置定时器  
    this->timer_updateFrame = new QTimer(this);  
    //- Qt::PreciseTimer -> 精确的精度, 毫秒级
    //- Qt::CoarseTimer  -> 粗糙的精度, 和1毫秒的误差在5%的范围内, 默认精度
    //- Qt::VeryCoarseTimer -> 非常粗糙的精度, 精度在1秒左右
    this->timer_updateFrame->setTimerType(Qt::PreciseTimer);
    connect(this->timer_updateFrame, 
            &QTimer::timeout, this, 
            &PlayerWidget::updateFrame);  

    this->timer_updateFrame->start(1000/fps); // 每500毫秒更新一次图片  
                                        //
    this->is_play = true;
}

void PlayerWidget::pushFrame2Show(const cv::Mat &mat)
{
    std::lock_guard<std::mutex> guard(this->images_lock);
    this->images << QPixmap::fromImage(MatToQImage2(mat));
}

void PlayerWidget::pushPixmap2Show(const QPixmap &pixmap)
{
    std::lock_guard<std::mutex> guard(this->images_lock);
    this->images << pixmap;
}
void PlayerWidget::startPlay(const int fps)
{
    if(this->is_play==false){
        this->timer_updateFrame->start(1000/fps); // 每500毫秒更新一次图片  
        this->is_play = true;
    }
    else if(this->is_play==true){
        this->stopPlay();
        this->startPlay(fps);
    }
}

void PlayerWidget::stopPlay()
{
    if(this->is_play==true){
        this->timer_updateFrame->stop(); // 每500毫秒更新一次图片  
        this->is_play = false;
    }
}

PlayerWidget::~PlayerWidget()
{
}

void PlayerWidget::paintEvent(QPaintEvent *event) {

    if (this->is_play == false) return;

#ifdef DEBUG_PLAYER_WIDGET_
    std::cout<<"<DEBUG>[paintEvent]images list size:"<<this->images.size()<<std::endl;
#endif


    QPainter painter(this);
    std::lock_guard<std::mutex> guard(this->images_lock);
    if (this->images.empty()) {
        if (!this->lastPixmap.isNull()) {
            painter.drawPixmap(rect(), this->lastPixmap);
        }
        return;
    }

    this->lastPixmap = this->images.front();
    painter.drawPixmap(rect(), this->lastPixmap);

    if (this->images.size() >= 2) {
        this->images.pop_front();
    } else if (this->images.size() == 1) {
        if (lockLastImg == false) {
            this->images.pop_front();
        }
    }

    while (this->images.size() > this->len_buf) {
        this->images.pop_front();
    }
#ifdef DEBUG_PLAYER_WIDGET_
        std::cout<<"<DEBUG>[paintEvent::Control Length]images list size:"<<images.size()<<std::endl;
#endif

}

void PlayerWidget::updateFrame()
{
    //currentImageIndex = (currentImageIndex + 1) % images.size(); // 更新索引并循环
    update(); // 请求重绘
}

//图像格式转换
QImage PlayerWidget::MatToQImage2(const cv::Mat &mat)
{
    QImage img;
    int chana = mat.channels();
    //依据通道数不同，改变不同的装换方式
    if(3 == chana ){
        cv::Mat tmp;
        if (is_rgb == true) {
            cv::cvtColor(mat, tmp, cv::COLOR_BGR2RGB);
        } else {
            tmp = mat;
        }
        img = QImage(static_cast<uchar *>(tmp.data), tmp.cols, tmp.rows, QImage::Format_RGB888).copy();
    }
    else if(4 == chana )
    {
        //argb
        img = QImage(static_cast<uchar *>(mat.data), mat.cols, mat.rows, QImage::Format_ARGB32).copy();
    }
    else {
    //单通道，灰度图
    img = QImage( mat.cols, mat.rows , QImage::Format_Indexed8);
    uchar * matdata = mat.data;
        for(int row = 0 ; row <mat.rows ; ++row )
        {
            uchar* rowdata = img.scanLine( row );
            memcpy(rowdata,matdata ,mat.cols);
        matdata+=mat.cols;
        }
    }
    return img;
}
