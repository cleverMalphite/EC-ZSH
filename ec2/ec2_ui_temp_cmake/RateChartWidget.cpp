#include "RateChartWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <cmath>

// ---- 调色板 ----
static const QColor kBg         { 15,  20,  35};   // 深蓝黑背景
static const QColor kGrid       { 40,  55,  80};   // 细格线
static const QColor kAxisText   {160, 180, 210};   // 轴标签
static const QColor kSendColor  { 96, 165, 250};   // 蓝 — 发送
static const QColor kRecvColor  {251, 146,  60};   // 橙 — 接收
static const QColor kFillSend   { 96, 165, 250,  45};
static const QColor kFillRecv   {251, 146,  60,  35};

// ---- 边距 ----
static constexpr int kML = 58;   // left  (Y轴标签)
static constexpr int kMR = 10;
static constexpr int kMT = 14;
static constexpr int kMB = 28;   // bottom (X轴标签)

// ============================================================

RateChartWidget::RateChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(130);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // 黑色背景，避免 paintEvent 前闪白
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(kBg));
    setPalette(pal);
    setAutoFillBackground(true);
}

void RateChartWidget::addSample(float sendKbps, float recvKbps)
{
    m_curSend = sendKbps;
    m_curRecv = recvKbps;
    m_sendSamples.push_back(sendKbps);
    m_recvSamples.push_back(recvKbps);
    while ((int)m_sendSamples.size() > kMaxSamples) m_sendSamples.pop_front();
    while ((int)m_recvSamples.size() > kMaxSamples) m_recvSamples.pop_front();
    update();
}

// ---- 辅助：格式化速率 ----
QString RateChartWidget::fmtRate(float kbps)
{
    if (kbps >= 1024.0f * 1024.0f)
        return QString::number(kbps / (1024.0f * 1024.0f), 'f', 2) + " Gbps";
    if (kbps >= 1024.0f)
        return QString::number(kbps / 1024.0f, 'f', 2) + " Mbps";
    return QString::number(kbps, 'f', 1) + " kbps";
}

// ---- 辅助：向上取整到好看的刻度 ----
float RateChartWidget::niceMax(float rawMax)
{
    if (rawMax <= 0.0f) return 1000.0f;      // 最低 1 Mbps 满量程
    // 找数量级，并取下一个 1/2/5 阶
    double mag = std::pow(10.0, std::floor(std::log10(rawMax)));
    for (double factor : {1.0, 2.0, 5.0, 10.0}) {
        double cand = mag * factor;
        if (cand >= rawMax * 1.1) return (float)cand;
    }
    return rawMax * 1.5f;
}

// ============================================================
void RateChartWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 绘图区域
    const QRect chartRect(kML, kMT, width() - kML - kMR, height() - kMT - kMB);
    if (chartRect.width() < 20 || chartRect.height() < 20) return;

    // ---- 背景 ----
    p.fillRect(rect(), kBg);

    // ---- 计算 Y 轴量程 ----
    float rawMax = 1.0f;
    for (float v : m_sendSamples) rawMax = qMax(rawMax, v);
    for (float v : m_recvSamples) rawMax = qMax(rawMax, v);
    const float yMax = niceMax(rawMax);

    // ---- 网格（5条横线）----
    const int kGridLines = 5;
    QFont smallFont = font();
    smallFont.setPointSize(8);
    p.setFont(smallFont);
    p.setPen(QPen(kGrid, 1, Qt::DotLine));
    for (int i = 0; i <= kGridLines; ++i) {
        float frac = (float)i / kGridLines;
        int y = chartRect.bottom() - (int)(frac * chartRect.height());
        p.setPen(QPen(kGrid, 1, Qt::DotLine));
        p.drawLine(chartRect.left(), y, chartRect.right(), y);
        // Y 轴标签
        p.setPen(kAxisText);
        QString label = fmtRate(yMax * frac);
        p.drawText(0, y - 8, kML - 4, 18,
                   Qt::AlignRight | Qt::AlignVCenter, label);
    }

    // ---- X 轴标签（时间，秒前）----
    p.setPen(kAxisText);
    const int n = (int)m_sendSamples.size();
    for (int sec : {0, 30, 60, 90, 120}) {
        int idx = n - 1 - sec;
        if (idx < 0) continue;
        float xFrac = (float)idx / (kMaxSamples - 1);
        int x = chartRect.left() + (int)(xFrac * chartRect.width());
        p.drawText(x - 20, chartRect.bottom() + 4, 40, kMB - 4,
                   Qt::AlignHCenter | Qt::AlignTop,
                   sec == 0 ? "Now" : QString("-%1s").arg(sec));
    }

    // ---- 绘制辅助 Lambda ----
    auto drawCurve = [&](const std::deque<float>& samples, QColor color, QColor fillColor) {
        if (samples.empty()) return;
        const int cnt = (int)samples.size();

        QPainterPath fillPath;
        QPainterPath linePath;
        bool first = true;
        for (int i = 0; i < cnt; ++i) {
            float xFrac = (float)i / (kMaxSamples - 1);
            float yFrac = (yMax > 0) ? (samples[i] / yMax) : 0.0f;
            yFrac = qBound(0.0f, yFrac, 1.0f);
            int px = chartRect.left() + (int)(xFrac * chartRect.width());
            int py = chartRect.bottom() - (int)(yFrac * chartRect.height());

            if (first) {
                fillPath.moveTo(px, chartRect.bottom());
                fillPath.lineTo(px, py);
                linePath.moveTo(px, py);
                first = false;
            } else {
                fillPath.lineTo(px, py);
                linePath.lineTo(px, py);
            }
        }
        // 闭合填充路径
        fillPath.lineTo(chartRect.left() + (int)(((float)(cnt-1)/(kMaxSamples-1)) * chartRect.width()),
                        chartRect.bottom());
        fillPath.closeSubpath();

        // 绘制填充
        p.setPen(Qt::NoPen);
        p.setBrush(fillColor);
        p.drawPath(fillPath);

        // 绘制线条
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(color, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(linePath);

        // 绘制端点圆
        if (cnt > 0) {
            float xFrac = (float)(cnt - 1) / (kMaxSamples - 1);
            float yFrac = (yMax > 0) ? (samples.back() / yMax) : 0.0f;
            yFrac = qBound(0.0f, yFrac, 1.0f);
            int px = chartRect.left() + (int)(xFrac * chartRect.width());
            int py = chartRect.bottom() - (int)(yFrac * chartRect.height());
            p.setPen(Qt::NoPen);
            p.setBrush(color);
            p.drawEllipse(QPoint(px, py), 4, 4);
        }
    };

    // 裁剪到绘图区
    p.setClipRect(chartRect);
    drawCurve(m_recvSamples, kRecvColor, kFillRecv);
    drawCurve(m_sendSamples, kSendColor, kFillSend);
    p.setClipping(false);

    // ---- 边框 ----
    p.setPen(QPen(kGrid.lighter(120), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(chartRect);

    // ---- 图例（右上角）----
    const int legX = chartRect.right() - 200;
    const int legY = chartRect.top() + 8;
    const int legH = 22;

    QFont legFont = font();
    legFont.setPointSize(9);
    legFont.setBold(true);
    p.setFont(legFont);

    // 发送图例
    p.setPen(Qt::NoPen);
    p.setBrush(kSendColor);
    p.drawRoundedRect(legX, legY + 4, 14, 4, 2, 2);
    p.setPen(kSendColor);
    p.drawText(legX + 18, legY, 180, legH, Qt::AlignLeft | Qt::AlignVCenter,
               QString("发送  %1").arg(fmtRate(m_curSend)));

    // 接收图例
    p.setPen(Qt::NoPen);
    p.setBrush(kRecvColor);
    p.drawRoundedRect(legX, legY + legH + 4, 14, 4, 2, 2);
    p.setPen(kRecvColor);
    p.drawText(legX + 18, legY + legH, 180, legH, Qt::AlignLeft | Qt::AlignVCenter,
               QString("接收  %1").arg(fmtRate(m_curRecv)));
}
