//
// Created by ubuntu402 on 24-5-18.
//

#ifndef EMERGENCYCOMMUNICATION_RESIZABLELABEL_H
#define EMERGENCYCOMMUNICATION_RESIZABLELABEL_H

#include <QLabel>
#include <QImage>
#include <QResizeEvent>
#include <QSizePolicy>

class ResizableLabel : public QLabel {
public:
    ResizableLabel(QWidget *parent = nullptr) : QLabel(parent) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
//    void resetPicture(){
//
//    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        int label_width=event->size().width();
        int label_height=event->size().height();
        int pic_width=image.width();
        int pic_height=image.height();
        int scale_width=label_width;
        int scale_height=label_height;
//        if(pic_width>label_width||pic_height>label_height) {
            scale_width = label_width;
            scale_height = pic_height * label_width / pic_width;
            if (scale_height > label_height) {
                scale_height = label_height;
                scale_width = pic_width * label_height / pic_height;
            }
//        }else{
//            scale_width=
//        }


        QSize scale_size(scale_width,scale_height);
        QImage scaledImage = image.scaled(scale_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QLabel::resizeEvent(event);
        setPixmap(QPixmap::fromImage(scaledImage));
    }

public:
    void setImage(QImage &img) {
        image = img;
        /*QSize size = this->size();
        QImage scaledImage = img.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        setPixmap(QPixmap::fromImage(scaledImage));*/
    }

private:
    QImage image;
};

#endif //EMERGENCYCOMMUNICATION_RESIZABLELABEL_H
