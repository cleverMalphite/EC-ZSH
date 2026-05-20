#include "SatelliteImageWidget.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QDebug>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

SatelliteImageWidget::SatelliteImageWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(160);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

bool SatelliteImageWidget::loadImage(const QString &imagePath)
{
    QStringList candidates;

    if (QFileInfo(imagePath).isAbsolute()) {
        candidates << imagePath;
    } else {
        const QString appDir = QApplication::applicationDirPath();
        candidates << QDir::current().absoluteFilePath(imagePath);
        candidates << QDir(appDir).absoluteFilePath(imagePath);
        candidates << QDir(appDir).absoluteFilePath("../" + imagePath);
        candidates << QDir(appDir).absoluteFilePath("../../" + imagePath);
    }

    // 常见兜底路径：ec2/build/ec2_autoconn 启动时，图片在 ../pic
    const QString appDir = QApplication::applicationDirPath();
    candidates << QDir(appDir).absoluteFilePath("../pic/OIP-C.webp");
    candidates << QDir(appDir).absoluteFilePath("../../ec2/pic/OIP-C.webp");

    // 去重
    candidates.removeDuplicates();

    for (const QString &path : candidates) {
        if (!QFileInfo::exists(path)) {
            continue;
        }

        // 1) 优先用 Qt 读（若系统带 webp 插件会直接成功）
        QImageReader reader(path);
        reader.setAutoTransform(true);
        QImage image = reader.read();
        if (!image.isNull()) {
            m_originalPixmap = QPixmap::fromImage(image);
            m_imagePath = path;
            update();
            qInfo() << "[SatelliteImageWidget] loaded by Qt:" << path;
            return true;
        }

        // 2) Qt 失败时，用 OpenCV 兜底（解决部分环境 webp 插件缺失）
        cv::Mat bgr = cv::imread(path.toStdString(), cv::IMREAD_COLOR);
        if (!bgr.empty()) {
            cv::Mat rgb;
            cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
            QImage cvImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);
            m_originalPixmap = QPixmap::fromImage(cvImage.copy());
            m_imagePath = path;
            update();
            qInfo() << "[SatelliteImageWidget] loaded by OpenCV:" << path;
            return true;
        }
    }

    qWarning() << "[SatelliteImageWidget] Failed to load image, candidates:" << candidates;
    return false;
}

void SatelliteImageWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!m_originalPixmap.isNull()) {
        const QPixmap scaled = m_originalPixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QRect targetRect(
            (width() - scaled.width()) / 2,
            (height() - scaled.height()) / 2,
            scaled.width(),
            scaled.height());
        painter.drawPixmap(targetRect, scaled);
    } else {
        painter.setPen(Qt::gray);
        QFont font = painter.font();
        font.setPointSize(14);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter, m_placeholderText);
    }
}

void SatelliteImageWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}
