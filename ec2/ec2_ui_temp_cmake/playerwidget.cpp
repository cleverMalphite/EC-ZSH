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
    this->images_lock.lock();
    this->images << QPixmap::fromImage(MatToQImage2(mat)) ;
    this->images_lock.unlock();
}

void PlayerWidget::pushPixmap2Show(const QPixmap &pixmap)
{
    this->images_lock.lock();
    this->images << pixmap;
    this->images_lock.unlock();
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

    if(this->images.empty() || this->is_play == false) return;

#ifdef DEBUG_PLAYER_WIDGET_
    std::cout<<"<DEBUG>[paintEvent]images list size:"<<this->images.size()<<std::endl;
#endif


    QPainter painter(this);
    //painter.drawPixmap(rect(), images[currentImageIndex]); // 绘制当前图片
    painter.drawPixmap(rect(), images.front()); // 绘制当前图片
    
    //Houlc 2024.05.24 set lock last img option
    if(this->images.size() >= 2){
        this->images_lock.lock();
        this->images.pop_front();
        this->images_lock.unlock();
    }
    else if(this->images.size() == 1){
        if(lockLastImg==true ){
                 
        }
        else if(lockLastImg==false){
            this->images_lock.lock();
            this->images.pop_front();
            this->images_lock.unlock();
        }
    }

    //2. control the size of images list,when too long
    if(this->images.size()>this->len_buf){
        this->images_lock.lock();
        for(int i = 0; i < this->images.size()-1; i++){
            this->images.pop_front();
            painter.drawPixmap(rect(), images.front()); // 绘制当前图片
        }
        this->images_lock.unlock();
#ifdef DEBUG_PLAYER_WIDGET_
        std::cout<<"<DEBUG>[paintEvent::Control Length]images list size:"<<images.size()<<std::endl;
#endif
    }

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
        //调整通道次序
        if(is_rgb==true)cv::cvtColor(mat,mat,cv::COLOR_BGR2RGB);
        img = QImage(static_cast<uchar *>(mat.data),mat.cols,mat.rows,QImage::Format_RGB888);
    }
    else if(4 == chana )
    {
        //argb
        img = QImage(static_cast<uchar *>(mat.data),mat.cols,mat.rows,QImage::Format_ARGB32);
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
