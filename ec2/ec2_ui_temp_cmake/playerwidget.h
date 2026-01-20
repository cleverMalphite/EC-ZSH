#ifndef PLAYER_WIDGET__
#define PLAYER_WIDGET__

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>

#include <mutex>

#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QVBoxLayout>
#include <QPushButton>

class PlayerWidget : public QWidget {
    Q_OBJECT

public:
    PlayerWidget(QWidget *parent = nullptr) ;
    ~PlayerWidget() ;

    void pushFrame2Show(const cv::Mat& mat);
    void pushPixmap2Show(const QPixmap &pixmap);

    void startPlay(const int fps);
    void stopPlay();

    void setMaxBufferSize(const int bufsize){
        if(bufsize>10) this->len_buf = 10;
        else this->len_buf = bufsize;
    }

    void setLockLastImg(bool islock){
        this->lockLastImg = islock;
    }

    int getFPS(){ return this->fps; }
    int getBufferSize(){ return images.size(); }
    int getMaxBufferSize(){ return this->len_buf; }

    void setRGBOrder(bool is_rgb){this->is_rgb=is_rgb;}

protected:
    void paintEvent(QPaintEvent *event) override ;

private slots:
    void updateFrame() ;

private:
    QImage MatToQImage2(const cv::Mat &mat);

    bool is_play ;

    bool is_rgb=true;

    bool lockLastImg = false;

    int fps;
    int len_buf;

    cv::VideoCapture video;
    QTimer *timer_readFrame;
    QTimer *timer_updateFrame;
 

    std::mutex images_lock;
    QList<QPixmap> images;
    QPixmap lastPixmap;
};


#endif
