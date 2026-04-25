#include "AutoConnWindow.h"
#include "ui_AutoConnWindow.h"
#include "function.h"

#include <QDateTime>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

// ──────────────────────────────────────────
//  状态显示配置表
// ──────────────────────────────────────────
struct StateStyle {
    QString dotColor;   // CSS color for the ● indicator
    QString cardBg;     // 卡片背景色
    QString bottomMsg;  // 底部状态栏文字
};

static StateStyle stateStyles[] = {
    // DISCONNECTED
    { "#999999", "#f0f4fa", "等待连接..." },
    // CONNECTING
    { "#3a8fea", "#e8f0fd", "正在建立连接，请稍候" },
    // CONNECTED
    { "#28c040", "#e8fce8", "通信链路正常" },
    // RECONNECTING
    { "#f5a623", "#fff8e8", "链路中断，正在重连" },
};

// ──────────────────────────────────────────
//  构造
// ──────────────────────────────────────────

AutoConnWindow::AutoConnWindow(AutoConnController *ctrl, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AutoConnWindow)
    , m_ctrl(ctrl)
{
    ui->setupUi(this);

    // ---- 初始化时钟定时器 ----
    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &AutoConnWindow::onClockTick);
    m_clockTimer->start(1000);
    onClockTick();  // 立即刷一次

    // ---- 连接控制器信号 ----
    connect(m_ctrl, &AutoConnController::stateChanged,
            this,   &AutoConnWindow::onStateChanged,   Qt::QueuedConnection);
    connect(m_ctrl, &AutoConnController::logMessage,
            this,   &AutoConnWindow::onLogMessage,     Qt::QueuedConnection);
    connect(m_ctrl, &AutoConnController::remoteTidChanged,
            this,   &AutoConnWindow::onRemoteTidChanged, Qt::QueuedConnection);
    connect(m_ctrl, &AutoConnController::transferRateUpdated,
            this,   &AutoConnWindow::onTransferRateUpdated, Qt::QueuedConnection);
    connect(m_ctrl, &AutoConnController::sendProgressUpdated,
            this,   &AutoConnWindow::onSendProgressUpdated, Qt::QueuedConnection);

    // ---- 手动重连按钮 ----
    connect(ui->reconnectBtn, &QPushButton::clicked,
            this, &AutoConnWindow::onReconnectBtnClicked);

    // ---- 传输按钮 ----
    connect(ui->browseBtn,   &QPushButton::clicked,
            this, &AutoConnWindow::onBrowseBtnClicked);
    connect(ui->sendFileBtn, &QPushButton::clicked,
            this, &AutoConnWindow::onSendFileBtnClicked);

    // ---- 填充静态信息 ----
    unsigned int tid = getTID();
    ui->tidLabel->setText(QString("本机 TID: %1").arg(tid));

    // 角色 & 地址信息从 ini 读（Controller 启动后已初始化）
    QString role  = (m_ctrl->role() == NodeRole::UAV) ? "无人机端 (UAV)" : "地面端 (GROUND)";
    ui->roleLabel->setText(QString("角色: %1").arg(role));

    QString localAddr  = QString::fromStdString(GetStringValueKeyIni("AutoConn", "LocalIP",   "127.0.0.1"))
                         + ":" + QString::number(GetIntegerKeyIni("AutoConn", "LocalPort",  3020));
    QString remoteAddr = QString::fromStdString(GetStringValueKeyIni("AutoConn", "RemoteIP",  "127.0.0.1"))
                         + ":" + QString::number(GetIntegerKeyIni("AutoConn", "RemotePort", 3021));

    ui->localAddrLabel->setText(localAddr);
    ui->remoteAddrLabel->setText(remoteAddr);

    // 初始状态
    applyStateStyle(ConnState::DISCONNECTED);
    ui->reconnectBtn->setEnabled(false); // 未断线前禁用
}

AutoConnWindow::~AutoConnWindow()
{
    cleanUp();
}

void AutoConnWindow::cleanUp()
{
    if (m_clockTimer) { m_clockTimer->stop(); }
    if (m_ctrl)       { m_ctrl->stop(); }
    System_shutdown(false);
    delete ui;
    ui = nullptr;
}

// ──────────────────────────────────────────
//  slots
// ──────────────────────────────────────────

void AutoConnWindow::onStateChanged(ConnState s)
{
    applyStateStyle(s);

    bool isConnected = (s == ConnState::CONNECTED);

    if (isConnected) {
        m_connectedAt = QDateTime::currentDateTime();
        m_timingConn  = true;
        ui->reconnectBtn->setEnabled(false);
    } else {
        m_timingConn = false;
        ui->uptimeLabel->setText("--");
        // 断线或重连中时开放手动重连
        ui->reconnectBtn->setEnabled(
            s == ConnState::DISCONNECTED || s == ConnState::RECONNECTING);
    }

    // 仅已连接且选了文件时才允许发送
    ui->sendFileBtn->setEnabled(isConnected && !m_selectedFilePath.isEmpty());
}

void AutoConnWindow::onLogMessage(const QString &msg)
{
    QString ts = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
    ui->logEdit->appendPlainText(ts + msg);
    // 自动滚动到底部
    QTextCursor c = ui->logEdit->textCursor();
    c.movePosition(QTextCursor::End);
    ui->logEdit->setTextCursor(c);
}

void AutoConnWindow::onRemoteTidChanged(unsigned int tid)
{
    if (tid == 0) {
        ui->remoteTidLabel->setText("--");
    } else {
        ui->remoteTidLabel->setText(QString::number(tid));
    }
}

void AutoConnWindow::onClockTick()
{
    ui->clockLabel->setText(
        QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    // 同步更新连接持续时间
    if (m_timingConn) {
        updateUptimeLabel();
    }
}

void AutoConnWindow::onReconnectBtnClicked()
{
    if (m_ctrl) {
        m_ctrl->requestReconnect();
    }
}

// ──────────────────────────────────────────
//  private helpers
// ──────────────────────────────────────────

void AutoConnWindow::applyStateStyle(ConnState s)
{
    int idx = (int)s;
    const StateStyle &st = stateStyles[idx];

    // 指示灯颜色
    ui->statusDot->setStyleSheet(
        QString("color: %1; font-size: 48px;").arg(st.dotColor));

    // 卡片背景
    ui->statusCard->setStyleSheet(
        QString("QFrame { background-color: %1; border-radius: 12px; border: 1px solid #d0daea; }")
            .arg(st.cardBg));

    // 底部状态栏
    ui->bottomStatusLabel->setText(st.bottomMsg);
}

void AutoConnWindow::updateUptimeLabel()
{
    qint64 secs = m_connectedAt.secsTo(QDateTime::currentDateTime());
    int h = (int)(secs / 3600);
    int m = (int)((secs % 3600) / 60);
    int sec = (int)(secs % 60);
    ui->uptimeLabel->setText(QString("%1:%2:%3")
                                 .arg(h,   2, 10, QChar('0'))
                                 .arg(m,   2, 10, QChar('0'))
                                 .arg(sec, 2, 10, QChar('0')));
}

// ──────────────────────────────────────────
//  数据传输相关 slots
// ──────────────────────────────────────────

void AutoConnWindow::onBrowseBtnClicked()
{
    QString path = QFileDialog::getOpenFileName(this, "选择要发送的文件", QDir::homePath());
    if (path.isEmpty()) return;
    m_selectedFilePath = path;
    ui->filePathEdit->setText(path);
    // 仅已连接时才激活发送按钮
    ui->sendFileBtn->setEnabled(m_ctrl->currentState() == ConnState::CONNECTED);
}

void AutoConnWindow::onSendFileBtnClicked()
{
    if (m_selectedFilePath.isEmpty()) return;

    QFileInfo fi(m_selectedFilePath);
    std::string filePath = m_selectedFilePath.toStdString();
    std::string fileName = fi.fileName().toStdString();

    bool ok = m_ctrl->sendFile(filePath, fileName);
    if (ok) {
        onLogMessage(QString("[Transfer] 已创建发送任务：%1").arg(fi.fileName()));
    } else {
        QMessageBox::warning(this, "发送失败", "创建文件传输任务失败，请确认连接正常。");
    }
}

void AutoConnWindow::onTransferRateUpdated(float sendKbps, float recvKbps)
{
    // 自动换算单位：>= 1024 kbps 时显示 Mbps
    auto fmtRate = [](float kbps) -> QString {
        if (kbps >= 1024.0f)
            return QString::number(kbps / 1024.0f, 'f', 2) + " Mbps";
        return QString::number(kbps, 'f', 1) + " kbps";
    };

    ui->sendRateLabel->setText(fmtRate(sendKbps));
    ui->recvRateLabel->setText(fmtRate(recvKbps));
}

void AutoConnWindow::onSendProgressUpdated(unsigned int taskId,
                                           const QString &filename,
                                           float rateKbps,
                                           unsigned int percent)
{
    QString msg = QString("[Transfer] 任务#%1 %2  %3%  %4 kbps")
                      .arg(taskId)
                      .arg(filename)
                      .arg(percent)
                      .arg(rateKbps, 0, 'f', 1);
    onLogMessage(msg);
}
