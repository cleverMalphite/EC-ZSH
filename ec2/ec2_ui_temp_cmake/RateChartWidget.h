#pragma once

#include <QWidget>
#include <QTimer>
#include <deque>

/**
 * RateChartWidget — 实时发送/接收速率折线图
 * 使用 QPainter 绘制，无需额外 Qt Modules。
 * 最多保存 kMaxSamples 个采样点（默认 120 个），横轴代表时间，纵轴自动缩放。
 */
class RateChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RateChartWidget(QWidget *parent = nullptr);

public slots:
    /** 添加一个新采样（由外部定时器每秒调用）。单位 kbps。 */
    void addSample(float sendKbps, float recvKbps);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // ---- 配置 ----
    static constexpr int kMaxSamples = 120;  // 保留 120 秒历史

    // ---- 数据 ----
    std::deque<float> m_sendSamples;
    std::deque<float> m_recvSamples;

    // ---- 当前显示值（用于图例） ----
    float m_curSend = 0.0f;
    float m_curRecv = 0.0f;

    // ---- 辅助 ----
    static QString fmtRate(float kbps);
    static float   niceMax(float rawMax);
};
