#ifndef SATELLITEIMAGEWIDGET_H
#define SATELLITEIMAGEWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QString>

/**
 * SatelliteImageWidget
 *
 * 静态图片展示组件，用于在顶部区域展示卫星遥感图片。
 * 支持等比例缩放、居中显示，预留加载本地/相对路径图片的接口。
 */
class SatelliteImageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SatelliteImageWidget(QWidget *parent = nullptr);

    // 加载指定路径的图片（支持绝对路径和相对于可执行文件的相对路径）
    bool loadImage(const QString &imagePath);

    // 获取当前加载的图片路径
    QString currentImagePath() const { return m_imagePath; }

    void setDefaultPlaceholderText(const QString &text) { m_placeholderText = text; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPixmap  m_originalPixmap;
    QString  m_imagePath;
    QString  m_placeholderText = "暂无图片";
};

#endif // SATELLITEIMAGEWIDGET_H
