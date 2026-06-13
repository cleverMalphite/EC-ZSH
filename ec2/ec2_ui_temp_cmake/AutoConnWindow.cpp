#include "AutoConnWindow.h"
#include "ui_AutoConnWindow.h"
#include "function.h"

#include "SatelliteImageWidget.h"
#include "SendHistoryTab.h"
#include "ReceiveHistoryTab.h"
#include "RealtimeStreamDialog.h"
#include "RateChartWidget.h"

#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QMap>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QButtonGroup>

struct StateStyle {
    QString cardBg;
    QString bottomMsg;
};

static StateStyle stateStyles[] = {
    { "#f0f4fa", "等待连接..." },
    { "#e8f0fd", "正在建立连接，请稍候" },
    { "#e8fce8", "通信链路正常" },
    { "#fff8e8", "链路中断，正在重连" },
};

AutoConnWindow::AutoConnWindow(AutoConnController *ctrl, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AutoConnWindow)
    , m_ctrl(ctrl)
{
    ui->setupUi(this);
    setupCustomWidgets();

    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &AutoConnWindow::onClockTick);
    m_clockTimer->start(1000);
    onClockTick();

    connect(m_ctrl, &AutoConnController::stateChanged,
            this,   &AutoConnWindow::onStateChanged,   Qt::QueuedConnection);
    connect(m_ctrl, &AutoConnController::logMessage,
            this,   &AutoConnWindow::onLogMessage,     Qt::QueuedConnection);
    connect(m_ctrl, &AutoConnController::remoteTidChanged,
            this,   &AutoConnWindow::onRemoteTidChanged, Qt::QueuedConnection);
    connect(m_ctrl, &AutoConnController::terminalListChanged,
            this,   &AutoConnWindow::onTerminalListChanged, Qt::QueuedConnection);
    connect(m_ctrl, &AutoConnController::transferRateUpdated,
            this,   &AutoConnWindow::onTransferRateUpdated, Qt::QueuedConnection);
    connect(m_ctrl, &AutoConnController::sendProgressUpdated,
            this,   &AutoConnWindow::onSendProgressUpdated, Qt::QueuedConnection);
    connect(m_ctrl, &AutoConnController::recvProgressUpdated,
            this,   &AutoConnWindow::onRecvProgressUpdated, Qt::QueuedConnection);

    connect(ui->reconnectBtn, &QPushButton::clicked,
            this, &AutoConnWindow::onReconnectBtnClicked);
    connect(ui->realtimeBtn, &QPushButton::clicked,
            this, &AutoConnWindow::onRealtimeBtnClicked);
    connect(ui->browseBtn,   &QPushButton::clicked,
            this, &AutoConnWindow::onBrowseBtnClicked);
    connect(ui->sendFileBtn, &QPushButton::clicked,
            this, &AutoConnWindow::onSendFileBtnClicked);
    connect(ui->targetTidCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { updateSendButtonEnabled(); });

    const unsigned int tid = getTID();
    ui->tidLabel->setText(QString("本机 TID: %1").arg(tid));

    ui->roleLabel->hide();
    ui->connectedTerminalsLabel->setText("--");
    ui->targetTidCombo->clear();

    const QString localAddr  = QString::fromStdString(GetStringValueKeyIni("AutoConn", "LocalIP",   "127.0.0.1"))
                               + ":" + QString::number(GetIntegerKeyIni("AutoConn", "LocalPort",  3020));
    const QString remoteAddr = QString::fromStdString(GetStringValueKeyIni("AutoConn", "RemoteIP",  "127.0.0.1"))
                               + ":" + QString::number(GetIntegerKeyIni("AutoConn", "RemotePort", 3021));

    ui->localAddrLabel->setText(localAddr);
    ui->remoteAddrLabel->setText(remoteAddr);

    // 初始化连接模式标签（从 ini 读取初始值）
    int connModeRaw = GetIntegerKeyIni("AutoConn", "ConnMode", -1);
    QString initModeText;
    if (connModeRaw == 1 || GetBoolValueKeyIni("AutoConn", "IsDTU", false))
        initModeText = QStringLiteral("DTU连接");
    else if (connModeRaw == 2)
        initModeText = QStringLiteral("多链路叠加");
    else
        initModeText = QStringLiteral("默认连接");
    ui->dtuModeLabel->setText(initModeText);

    applyStateStyle(ConnState::DISCONNECTED);
    ui->reconnectBtn->setEnabled(m_ctrl != nullptr);
    if (m_ctrl) {
        onStateChanged(m_ctrl->currentState());
    }
    updateSendButtonEnabled();
}

AutoConnWindow::~AutoConnWindow()
{
    cleanUp();
}

void AutoConnWindow::cleanUp()
{
    if (m_realtimeDlg) { m_realtimeDlg->close(); delete m_realtimeDlg; m_realtimeDlg = nullptr; }
    if (m_clockTimer) { m_clockTimer->stop(); }
    if (m_ctrl)       { m_ctrl->stop(); }
    System_shutdown(false);
    delete ui;
    ui = nullptr;
}

void AutoConnWindow::setupCustomWidgets()
{
    // ---- 1) 顶部图片组件 ----
    m_imageWidget = new SatelliteImageWidget(this);
    auto *imageLayout = new QVBoxLayout(ui->imageContainer);
    imageLayout->setContentsMargins(0, 0, 0, 0);
    imageLayout->addWidget(m_imageWidget);

    bool ok = m_imageWidget->loadImage(QStringLiteral("../pic/OIP.webp"));
    if (!ok) {
        ok = m_imageWidget->loadImage(QStringLiteral("pic/OIP.webp"));
    }
    if (!ok) {
        ok = m_imageWidget->loadImage(QStringLiteral("../../pic/OIP.webp"));
    }
    if (!ok) {
        // 兜底：用可执行文件所在目录定位
        const QString appDir = QCoreApplication::applicationDirPath();
        ok = m_imageWidget->loadImage(QDir(appDir).absoluteFilePath("../pic/OIP.webp"));
    }

    // ---- 2) 速率图 Tab（第一个 Tab）----
    m_rateChart = new RateChartWidget(this);
    ui->infoTabWidget->addTab(m_rateChart, QStringLiteral("速率图"));

    // 将 Controller 的速率信号接入图表
    if (m_ctrl) {
        connect(m_ctrl, &AutoConnController::transferRateUpdated,
                m_rateChart, &RateChartWidget::addSample,
                Qt::QueuedConnection);
    }

    // ---- 3) 运行日志 Tab ----
    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(300);
    m_logEdit->setStyleSheet(
        R"(
        QPlainTextEdit {
            background-color: #f8fafc;
            color: #333333;
            font-family: Monospace;
            font-size: 12px;
            border: none;
        }
        )"
    );
    ui->infoTabWidget->addTab(m_logEdit, QStringLiteral("运行日志"));

    m_sendHistoryTab = new SendHistoryTab(this);
    ui->infoTabWidget->addTab(m_sendHistoryTab, QStringLiteral("发送历史"));

    m_recvHistoryTab = new ReceiveHistoryTab(this);
    ui->infoTabWidget->addTab(m_recvHistoryTab, QStringLiteral("接收历史"));

    ui->infoTabWidget->setCurrentIndex(0);

    // ---- 5) 网络类型按钮互斥 ----
    auto *netBtnGroup = new QButtonGroup(this);
    netBtnGroup->addButton(ui->meshBtn);
    netBtnGroup->addButton(ui->wifiBtn);
    netBtnGroup->addButton(ui->fiveGBtn);
    netBtnGroup->addButton(ui->satelliteBtn);
    netBtnGroup->setExclusive(true);
}

void AutoConnWindow::onStateChanged(ConnState s)
{
    applyStateStyle(s);

    const bool isConnected = (s == ConnState::CONNECTED);
    if (isConnected) {
        m_connectedAt = QDateTime::currentDateTime();
        m_timingConn  = true;
    } else {
        m_timingConn = false;
        ui->uptimeLabel->setText("--");
    }

    ui->reconnectBtn->setEnabled(m_ctrl != nullptr);
    updateSendButtonEnabled();
}

QString AutoConnWindow::formatDuration(qint64 elapsedMs)
{
    if (elapsedMs < 1000) {
        return QString("%1 ms").arg(elapsedMs);
    }
    return QString("%1 s").arg(elapsedMs / 1000.0, 0, 'f', 2);
}

void AutoConnWindow::syncHistoryFromLogLine(const QString &msg)
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    // 发送失败日志: [Transfer] 任务#1000000 file.txt 发送失败
    {
        static const QRegularExpression re(R"(^\[Transfer\]\s*任务#(\d+)\s+(.+?)\s+发送失败$)");
        const QRegularExpressionMatch m = re.match(msg);
        if (m.hasMatch()) {
            const unsigned int taskId = m.captured(1).toUInt();
            const QString fileName = m.captured(2).trimmed();
            const qint64 startMs = m_sendStartMs.take(taskId);
            const QString duration = (startMs > 0) ? formatDuration(nowMs - startMs) : QString();
            m_sendHistoryTab->upsertSendProgress(taskId, 0, fileName, 0,
                                                 QStringLiteral("发送失败"), 0.0f, duration);
            return;
        }
    }

    // 发送完成日志: [Transfer] 任务#1000000 file.txt 发送完成 100%  平均 329.5 kbps
    {
        static const QRegularExpression re(R"(^\[Transfer\]\s*任务#(\d+)\s+(.+?)\s+发送完成\s+100%\s+平均\s+([0-9.]+)\s+kbps$)");
        const QRegularExpressionMatch m = re.match(msg);
        if (m.hasMatch()) {
            const unsigned int taskId = m.captured(1).toUInt();
            const QString fileName = m.captured(2).trimmed();
            const float rate = m.captured(3).toFloat();
            const qint64 startMs = m_sendStartMs.take(taskId);
            const QString duration = (startMs > 0) ? formatDuration(nowMs - startMs) : QString();
            m_sendHistoryTab->upsertSendProgress(taskId, 0, fileName, 0,
                                                 QStringLiteral("发送成功"), rate, duration);
            return;
        }
    }

    // 接收进度日志: [Transfer] 接收任务#123 xx.bin  20%  188.0 kbps
    {
        static const QRegularExpression re(R"(^\[Transfer\]\s*接收任务#(\d+)\s+(.+?)\s+(\d+)%\s+([0-9.]+)\s+kbps$)");
        const QRegularExpressionMatch m = re.match(msg);
        if (m.hasMatch()) {
            const unsigned int taskId = m.captured(1).toUInt();
            const QString fileName = m.captured(2).trimmed();
            const unsigned int pct = m.captured(3).toUInt();
            const float rate = m.captured(4).toFloat();
            if (!m_recvStartMs.contains(taskId)) {
                m_recvStartMs.insert(taskId, nowMs);
            }
            const QString status = (pct >= 100) ? QStringLiteral("接收完成") : QStringLiteral("接收中");
            QString duration;
            if (pct >= 100) {
                const qint64 startMs = m_recvStartMs.take(taskId);
                duration = (startMs > 0) ? formatDuration(nowMs - startMs) : QString();
            }
            m_recvHistoryTab->upsertRecvProgress(taskId, 0, fileName, 0, status, rate, duration);
            return;
        }
    }

    // 接收状态日志: [Transfer] 接收任务#123 xx.bin  Recv Success/Failed
    {
        static const QRegularExpression re(R"(^\[Transfer\]\s*接收任务#(\d+)\s+(.+?)\s+Recv\s+(Success|Failed)$)");
        const QRegularExpressionMatch m = re.match(msg);
        if (m.hasMatch()) {
            const unsigned int taskId = m.captured(1).toUInt();
            const QString fileName = m.captured(2).trimmed();
            const QString flag = m.captured(3);
            const QString status = (flag == QStringLiteral("Success"))
                ? QStringLiteral("接收成功")
                : QStringLiteral("接收失败");
            const qint64 startMs = m_recvStartMs.take(taskId);
            const QString duration = (startMs > 0) ? formatDuration(nowMs - startMs) : QString();
            m_recvHistoryTab->upsertRecvProgress(taskId, 0, fileName, 0, status, -1.0f, duration);
            return;
        }
    }
}

void AutoConnWindow::onLogMessage(const QString &msg)
{
    const QString ts = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
    m_logEdit->appendPlainText(ts + msg);

    QTextCursor c = m_logEdit->textCursor();
    c.movePosition(QTextCursor::End);
    m_logEdit->setTextCursor(c);

    syncHistoryFromLogLine(msg);
}

void AutoConnWindow::onRemoteTidChanged(unsigned int tid)
{
    if (tid == 0) {
        ui->remoteTidLabel->setText("--");
    } else {
        ui->remoteTidLabel->setText(QString::number(tid));
    }
}

void AutoConnWindow::onTerminalListChanged(const QStringList &tids)
{
    const QString current = ui->targetTidCombo->currentText();

    ui->targetTidCombo->clear();
    ui->targetTidCombo->addItems(tids);

    if (!current.isEmpty()) {
        const int idx = ui->targetTidCombo->findText(current);
        if (idx >= 0) {
            ui->targetTidCombo->setCurrentIndex(idx);
        }
    }

    if (ui->targetTidCombo->count() > 0) {
        ui->connectedTerminalsLabel->setText(tids.join(", "));
        if (ui->remoteTidLabel->text().isEmpty() || ui->remoteTidLabel->text() == "--") {
            ui->remoteTidLabel->setText(ui->targetTidCombo->itemText(0));
        }
    } else {
        ui->connectedTerminalsLabel->setText("--");
        ui->remoteTidLabel->setText("--");
    }

    updateSendButtonEnabled();
}

void AutoConnWindow::onClockTick()
{
    ui->clockLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    if (m_timingConn) {
        updateUptimeLabel();
    }
}

void AutoConnWindow::onReconnectBtnClicked()
{
    if (!m_ctrl) {
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("手动连接设置"));

    auto *layout = new QVBoxLayout(&dlg);
    auto *form = new QFormLayout();

    // ---- 自组网链路参数 ----
    auto *localIpEdit = new QLineEdit(m_ctrl->localAddr(), &dlg);
    auto *localPortSpin = new QSpinBox(&dlg);
    localPortSpin->setRange(0, 65535);
    localPortSpin->setValue(m_ctrl->localPort());

    auto *remoteIpEdit = new QLineEdit(m_ctrl->remoteAddr(), &dlg);
    auto *remotePortSpin = new QSpinBox(&dlg);
    remotePortSpin->setRange(1, 65535);
    remotePortSpin->setValue(m_ctrl->remotePort());

    // ---- 连接模式选择（三选一）----
    auto *modeCombo = new QComboBox(&dlg);
    modeCombo->addItem(QStringLiteral("仅自组网"),  (int)ConnMode::MeshOnly);
    modeCombo->addItem(QStringLiteral("仅DTU"),     (int)ConnMode::DTUOnly);
    modeCombo->addItem(QStringLiteral("多链路叠加"), (int)ConnMode::DualPath);
    modeCombo->setCurrentIndex(static_cast<int>(m_ctrl->connMode()));

    // ---- DTU 链路参数（仅在 DTUOnly / DualPath 时显示）----
    auto *dtuLocalIpEdit    = new QLineEdit(m_ctrl->dtuLocalAddr(), &dlg);
    auto *dtuLocalPortSpin  = new QSpinBox(&dlg);
    dtuLocalPortSpin->setRange(0, 65535);
    dtuLocalPortSpin->setValue(m_ctrl->dtuLocalPort());

    auto *dtuRemoteIpEdit   = new QLineEdit(m_ctrl->dtuRemoteAddr(), &dlg);
    auto *dtuRemotePortSpin = new QSpinBox(&dlg);
    dtuRemotePortSpin->setRange(1, 65535);
    dtuRemotePortSpin->setValue(m_ctrl->dtuRemotePort());

    // DTU 行标签和控件引用，用于显示/隐藏
    QLabel *dtuLocalLabel  = new QLabel(QStringLiteral("DTU本地 IP："), &dlg);
    QLabel *dtuLocalPLabel = new QLabel(QStringLiteral("DTU本地端口："), &dlg);
    QLabel *dtuRemoteLabel  = new QLabel(QStringLiteral("DTU对端 IP："), &dlg);
    QLabel *dtuRemotePLabel = new QLabel(QStringLiteral("DTU对端端口："), &dlg);

    auto toggleDtuFields = [=](bool visible) {
        dtuLocalLabel->setVisible(visible);
        dtuLocalIpEdit->setVisible(visible);
        dtuLocalPLabel->setVisible(visible);
        dtuLocalPortSpin->setVisible(visible);
        dtuRemoteLabel->setVisible(visible);
        dtuRemoteIpEdit->setVisible(visible);
        dtuRemotePLabel->setVisible(visible);
        dtuRemotePortSpin->setVisible(visible);
    };

    // 初始状态
    const bool showDtu = (modeCombo->currentIndex() != 0);  // 非 MeshOnly 时显示
    toggleDtuFields(showDtu);

    QObject::connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     &dlg, [=](int idx) { toggleDtuFields(idx != 0); });

    form->addRow(QStringLiteral("连接模式："), modeCombo);
    form->addRow(QStringLiteral("本地 IP："), localIpEdit);
    form->addRow(QStringLiteral("本地端口："), localPortSpin);
    form->addRow(QStringLiteral("对端 IP："), remoteIpEdit);
    form->addRow(QStringLiteral("对端端口："), remotePortSpin);

    // DTU 参数行
    form->addRow(dtuLocalLabel, dtuLocalIpEdit);
    form->addRow(dtuLocalPLabel, dtuLocalPortSpin);
    form->addRow(dtuRemoteLabel, dtuRemoteIpEdit);
    form->addRow(dtuRemotePLabel, dtuRemotePortSpin);

    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    const QString localIp = localIpEdit->text().trimmed();
    const QString remoteIp = remoteIpEdit->text().trimmed();
    if (localIp.isEmpty() || remoteIp.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("参数错误"), QStringLiteral("本地IP和对端IP不能为空。"));
        return;
    }

    // DTU 模式下验证 DTU 参数
    const ConnMode mode = static_cast<ConnMode>(modeCombo->currentData().toInt());
    const QString dtuLocalIp  = dtuLocalIpEdit->text().trimmed();
    const QString dtuRemoteIp = dtuRemoteIpEdit->text().trimmed();
    if (mode == ConnMode::DTUOnly || mode == ConnMode::DualPath) {
        if (dtuLocalIp.isEmpty() || dtuRemoteIp.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("参数错误"),
                                 QStringLiteral("DTU模式下，DTU本地IP和对端IP不能为空。"));
            return;
        }
    }

    m_ctrl->applyManualConnection(localIp,
                                  localPortSpin->value(),
                                  remoteIp,
                                  remotePortSpin->value(),
                                  mode,
                                  dtuLocalIp,
                                  dtuLocalPortSpin->value(),
                                  dtuRemoteIp,
                                  dtuRemotePortSpin->value());

    ui->localAddrLabel->setText(QString("%1:%2").arg(localIp).arg(localPortSpin->value()));
    ui->remoteAddrLabel->setText(QString("%1:%2").arg(remoteIp).arg(remotePortSpin->value()));

    // 更新模式标签
    QString modeLabel;
    switch (mode) {
        case ConnMode::MeshOnly: modeLabel = QStringLiteral("默认连接"); break;
        case ConnMode::DTUOnly:  modeLabel = QStringLiteral("DTU连接"); break;
        case ConnMode::DualPath: modeLabel = QStringLiteral("多链路叠加"); break;
    }
    ui->dtuModeLabel->setText(modeLabel);
}

void AutoConnWindow::onRealtimeBtnClicked()
{
    if (!m_realtimeDlg) {
        m_realtimeDlg = new RealtimeStreamDialog(m_ctrl, this);
        m_realtimeDlg->setAttribute(Qt::WA_DeleteOnClose, false);
    }

    // Sync current terminal list into the dialog
    QStringList tids;
    for (int i = 0; i < ui->targetTidCombo->count(); ++i) {
        tids << ui->targetTidCombo->itemText(i);
    }
    if (!tids.isEmpty()) {
        m_realtimeDlg->setTerminalList(tids);
    }

    m_realtimeDlg->show();
    m_realtimeDlg->raise();
    m_realtimeDlg->activateWindow();
}

void AutoConnWindow::applyStateStyle(ConnState s)
{
    const int idx = static_cast<int>(s);
    const StateStyle &st = stateStyles[idx];

    static QMap<ConnState, QString> borderColorMap = {
        { ConnState::DISCONNECTED,  QStringLiteral("#c8d4e8") },
        { ConnState::CONNECTING,    QStringLiteral("#3a8fea") },
        { ConnState::CONNECTED,     QStringLiteral("#7ec850") },
        { ConnState::RECONNECTING,  QStringLiteral("#f5a623") },
    };

    const QString borderClr = borderColorMap.value(s, QStringLiteral("#7ec850"));
    ui->imageCard->setStyleSheet(
        QString("QFrame { background-color: #f0f4fa; border-radius: 10px; border: 2px solid %1; }").arg(borderClr));

    ui->infoCard->setStyleSheet(
        QString("QFrame { background-color: %1; border-radius: 8px; border: 1px solid #d0daea; }").arg(st.cardBg));

    ui->bottomStatusLabel->setText(st.bottomMsg);
}

void AutoConnWindow::updateUptimeLabel()
{
    const qint64 secs = m_connectedAt.secsTo(QDateTime::currentDateTime());
    const int h = static_cast<int>(secs / 3600);
    const int m = static_cast<int>((secs % 3600) / 60);
    const int sec = static_cast<int>(secs % 60);
    ui->uptimeLabel->setText(QString("%1:%2:%3")
                                 .arg(h,   2, 10, QChar('0'))
                                 .arg(m,   2, 10, QChar('0'))
                                 .arg(sec, 2, 10, QChar('0')));
}

unsigned int AutoConnWindow::currentRemoteTid() const
{
    bool ok = false;
    const unsigned int tid = ui->remoteTidLabel->text().toUInt(&ok);
    return ok ? tid : 0;
}

unsigned int AutoConnWindow::selectedTargetTid() const
{
    bool ok = false;
    const unsigned int tid = ui->targetTidCombo->currentText().toUInt(&ok);
    return ok ? tid : 0;
}

void AutoConnWindow::onBrowseBtnClicked()
{
    const QStringList paths = QFileDialog::getOpenFileNames(this, "选择要发送的文件", QDir::homePath());
    if (paths.isEmpty()) {
        return;
    }

    m_selectedFilePaths = paths;
    if (paths.size() == 1) {
        ui->filePathEdit->setText(paths.first());
    } else {
        ui->filePathEdit->setText(QString("已选择 %1 个文件").arg(paths.size()));
    }
    updateSendButtonEnabled();
}

void AutoConnWindow::onSendFileBtnClicked()
{
    if (m_selectedFilePaths.isEmpty()) {
        return;
    }

    const unsigned int targetTid = selectedTargetTid();
    if (targetTid == 0) {
        QMessageBox::warning(this, "发送失败", "请先选择目标终端。\n若未出现目标终端，请确认已建立连接。");
        return;
    }

    int successCount = 0;
    int failCount = 0;
    for (const QString &filePath : m_selectedFilePaths) {
        QFileInfo fi(filePath);
        const std::string stdFilePath = filePath.toStdString();
        const std::string stdFileName = fi.fileName().toStdString();

        const bool ok = m_ctrl->sendFileToTid(targetTid, stdFilePath, stdFileName);
        if (ok) {
            onLogMessage(QString("[Transfer] 已创建发送任务：%1 -> TID %2")
                         .arg(fi.fileName())
                         .arg(targetTid));
            ++successCount;
        } else {
            onLogMessage(QString("[Transfer] 发送任务创建失败：%1").arg(fi.fileName()));
            ++failCount;
        }
    }

    if (failCount > 0 && successCount > 0) {
        QMessageBox::warning(this, "部分发送失败",
                             QString("成功创建 %1 个任务，%2 个失败。").arg(successCount).arg(failCount));
    } else if (failCount > 0) {
        QMessageBox::warning(this, "发送失败", "创建文件传输任务失败，请确认连接正常。");
    }
}

void AutoConnWindow::onTransferRateUpdated(float sendKbps, float recvKbps)
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    if (sendKbps > 0.0f) {
        m_lastSendRateKbps = sendKbps;
        m_lastSendRateMs = nowMs;
    }
    if (recvKbps > 0.0f) {
        m_lastRecvRateKbps = recvKbps;
        m_lastRecvRateMs = nowMs;
    }

    const bool sendFresh = (m_lastSendRateMs > 0) && ((nowMs - m_lastSendRateMs) <= 2500);
    const bool recvFresh = (m_lastRecvRateMs > 0) && ((nowMs - m_lastRecvRateMs) <= 2500);
    const float showSend = (sendKbps > 0.0f) ? sendKbps : (sendFresh ? m_lastSendRateKbps : 0.0f);
    const float showRecv = (recvKbps > 0.0f) ? recvKbps : (recvFresh ? m_lastRecvRateKbps : 0.0f);

    auto fmtRate = [](float kbps) -> QString {
        if (kbps >= 1024.0f)
            return QString::number(kbps / 1024.0f, 'f', 2) + " Mbps";
        return QString::number(kbps, 'f', 1) + " kbps";
    };

    ui->sendRateLabel->setText(fmtRate(showSend));
    ui->recvRateLabel->setText(fmtRate(showRecv));
}

void AutoConnWindow::onSendProgressUpdated(unsigned int taskId,
                                           unsigned int receiverTid,
                                           const QString &filename,
                                           quint64 fileSize,
                                           float rateKbps,
                                           unsigned int percent)
{
    if (rateKbps > 0.0f) {
        m_lastSendRateKbps = rateKbps;
        m_lastSendRateMs = QDateTime::currentMSecsSinceEpoch();
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    // 首次出现该任务时记录开始时间
    if (!m_sendStartMs.contains(taskId)) {
        m_sendStartMs.insert(taskId, nowMs);
    }

    const QString status = (percent >= 100) ? QStringLiteral("发送成功") : QStringLiteral("发送中");
    QString duration;
    if (percent >= 100) {
        const qint64 startMs = m_sendStartMs.take(taskId);
        duration = (startMs > 0) ? formatDuration(nowMs - startMs) : QString();
    }
    m_sendHistoryTab->upsertSendProgress(taskId,
                                         receiverTid,
                                         filename,
                                         fileSize,
                                         status,
                                         rateKbps,
                                         duration);
}

void AutoConnWindow::onRecvProgressUpdated(unsigned int taskId,
                                           unsigned int senderTid,
                                           const QString &filename,
                                           quint64 fileSize,
                                           float rateKbps,
                                           unsigned int percent)
{
    if (rateKbps > 0.0f) {
        m_lastRecvRateKbps = rateKbps;
        m_lastRecvRateMs = QDateTime::currentMSecsSinceEpoch();
    }

    auto fmtRate = [](float kbps) -> QString {
        if (kbps >= 1024.0f)
            return QString::number(kbps / 1024.0f, 'f', 2) + " Mbps";
        return QString::number(kbps, 'f', 1) + " kbps";
    };
    ui->recvRateLabel->setText(fmtRate((rateKbps > 0.0f) ? rateKbps : m_lastRecvRateKbps));

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    // 首次出现该任务时记录开始时间
    if (!m_recvStartMs.contains(taskId)) {
        m_recvStartMs.insert(taskId, nowMs);
    }

    const QString status = (percent >= 100) ? QStringLiteral("接收完成") : QStringLiteral("接收中");
    QString duration;
    if (percent >= 100) {
        const qint64 startMs = m_recvStartMs.take(taskId);
        duration = (startMs > 0) ? formatDuration(nowMs - startMs) : QString();
    }
    m_recvHistoryTab->upsertRecvProgress(taskId,
                                         senderTid,
                                         filename,
                                         fileSize,
                                         status,
                                         rateKbps,
                                         duration);
}

void AutoConnWindow::updateSendButtonEnabled()
{
    const bool connected = (m_ctrl && m_ctrl->currentState() == ConnState::CONNECTED);
    const bool hasFile = !m_selectedFilePaths.isEmpty();
    const bool hasTarget = (selectedTargetTid() != 0);
    ui->sendFileBtn->setEnabled(connected && hasFile && hasTarget);
}
