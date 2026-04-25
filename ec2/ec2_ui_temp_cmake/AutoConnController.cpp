#include "AutoConnController.h"
#include "function.h"

#include <QDateTime>
#include <QDebug>
#include <QString>
#include "../BigDataTransfer/BigDataTransferApi.h"

// 引用 function.cpp 里的全局终端列表和任务状态
extern std::vector<std::shared_ptr<ClientTemplate>> _clientTemplate;
extern std::vector<std::shared_ptr<SendTaskState>>  _sendTaskState;
extern std::vector<std::shared_ptr<RecvTaskState>>  _recvTaskState;
extern pthread_mutex_t _terminal_Mutex;
extern pthread_mutex_t _sendTask_Mutex;
extern pthread_mutex_t _recvTask_Mutex;
extern DataUpdater *dataUpdater;

// ============================================================
//  构造 / 析构
// ============================================================

AutoConnController::AutoConnController(QObject *parent)
    : QObject(parent)
{
    m_retryTimer     = new QTimer(this);
    m_heartbeatTimer = new QTimer(this);
    m_rateTimer      = new QTimer(this);

    m_retryTimer->setSingleShot(false);
    m_heartbeatTimer->setSingleShot(false);
    m_rateTimer->setSingleShot(false);

    connect(m_retryTimer,     &QTimer::timeout, this, &AutoConnController::onRetryTimer);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &AutoConnController::onHeartbeatTimer);
    connect(m_rateTimer,      &QTimer::timeout, this, &AutoConnController::onRateTimer);
}

AutoConnController::~AutoConnController()
{
    stop();
}

// ============================================================
//  public
// ============================================================

void AutoConnController::start()
{
    if (m_stopped) return;

    readConfig();

    emit logMessage(QString("[AutoConn] 角色: %1  本地 %2:%3  远端 %4:%5")
                        .arg(m_role == NodeRole::UAV ? "无人机端(UAV)" : "地面端(GROUND)")
                        .arg(QString::fromStdString(m_localAddr))
                        .arg(m_localPort)
                        .arg(QString::fromStdString(m_remoteAddr))
                        .arg(m_remotePort));

    // 监听终端列表变化
    if (dataUpdater) {
        connect(dataUpdater, &DataUpdater::updateTerminalSignal,
                this, &AutoConnController::onTerminalListUpdated,
                Qt::QueuedConnection);

        // 连接发送进度信号，将底层进度转发给 UI
        connect(dataUpdater, &DataUpdater::updateSendProgressSignal,
                this, [this](unsigned int task_id) {
                    MutexLockGuard guard(_sendTask_Mutex);
                    for (const auto &s : _sendTaskState) {
                        if (s && s->_taskID == task_id) {
                            float kbps = s->_transferSpeed;
                            unsigned int pct = s->_process;
                            QString name = QString::fromStdString(s->_taskName);
                            unsigned int state = s->_taskState;
                            emit sendProgressUpdated(task_id, name, kbps, pct);
                            // 发送完成/失败时补打结果日志
                            if (state == 1) {
                                emit logMessage(QString("[Transfer] 任务#%1 %2 发送完成 100%  平均 %3 kbps")
                                    .arg(task_id).arg(name).arg(kbps, 0, 'f', 1));
                            } else if (state == 2) {
                                emit logMessage(QString("[Transfer] 任务#%1 %2 发送失败")
                                    .arg(task_id).arg(name));
                            }
                            break;
                        }
                    }
                }, Qt::QueuedConnection);

        // 接收进度信号
        connect(dataUpdater, &DataUpdater::updateRecvProgressSignal,
                this, [this](unsigned int task_id) {
                    MutexLockGuard guard(_recvTask_Mutex);
                    for (const auto &r : _recvTaskState) {
                        if (r && r->_taskID == task_id) {
                            emit logMessage(QString("[Transfer] 接收任务#%1 %2  %3%  %4 kbps")
                                .arg(task_id)
                                .arg(QString::fromStdString(r->_taskName))
                                .arg(r->_process)
                                .arg(r->_transferSpeed, 0, 'f', 1));
                            break;
                        }
                    }
                }, Qt::QueuedConnection);

        // 接收完成/失败时打印日志
        connect(dataUpdater, &DataUpdater::updateRecvStatusSignal,
                this, [this]() {
                    MutexLockGuard guard(_recvTask_Mutex);
                    for (const auto &r : _recvTaskState) {
                        if (!r) continue;
                        // FILE_RECV_TASK_SUCCESS=5, FILE_RECV_TASK_FAILED=6
                        if (r->_taskState == 5) {
                            emit logMessage(QString("[Transfer] 接收任务#%1 %2  Recv Success")
                                .arg(r->_taskID)
                                .arg(QString::fromStdString(r->_taskName)));
                        } else if (r->_taskState == 6) {
                            emit logMessage(QString("[Transfer] 接收任务#%1 %2  Recv Failed")
                                .arg(r->_taskID)
                                .arg(QString::fromStdString(r->_taskName)));
                        }
                    }
                }, Qt::QueuedConnection);
    }

    // 启动心跳检测（双端均需要）
    m_heartbeatTimer->start(1000);  // 每秒检查一次
    m_rateTimer->start(1000);       // 每秒轮询传输速率

    if (m_role == NodeRole::UAV) {
        // 无人机端：立即开始监听
        doStartListening();
    } else {
        // 地面端：立即发起第一次连接
        setState(ConnState::CONNECTING);
        doConnect();
        // 再启动重试定时器（连接成功后会停止）
        m_retryTimer->start(m_retryIntervalMs);
    }

    // 启动完成后立即执行一次心跳检查
    // 防止底层在 start() 之前就已连接而信号未到达导致 UI 停在 DISCONNECTED
    QTimer::singleShot(200, this, &AutoConnController::onHeartbeatTimer);
}

void AutoConnController::stop()
{
    m_stopped = true;
    if (m_retryTimer)     { m_retryTimer->stop(); }
    if (m_heartbeatTimer) { m_heartbeatTimer->stop(); }
    if (m_rateTimer)      { m_rateTimer->stop(); }
}

void AutoConnController::requestReconnect()
{
    if (m_state == ConnState::CONNECTED) return;
    emit logMessage("[AutoConn] 手动触发重连");
    doReconnect();
}

// ============================================================
//  private slots
// ============================================================

void AutoConnController::onRetryTimer()
{
    // 仅地面端使用；已连接时停止重试
    if (m_state == ConnState::CONNECTED) {
        m_retryTimer->stop();
        return;
    }
    emit logMessage(QString("[AutoConn] 重试连接 → %1:%2")
                        .arg(QString::fromStdString(m_remoteAddr))
                        .arg(m_remotePort));
    doConnect();
}

void AutoConnController::onHeartbeatTimer()
{
    if (m_stopped) return;

    bool nowConnected = isConnected();

    if (nowConnected) {
        // 更新心跳时间戳
        m_lastSeenMs = QDateTime::currentMSecsSinceEpoch();

        if (m_state != ConnState::CONNECTED) {
            unsigned int tid = getActiveTid();
            m_remoteTid = tid;
            setState(ConnState::CONNECTED);
            // 连接成功后地面端停止重试
            if (m_role == NodeRole::GROUND) {
                m_retryTimer->stop();
            }
            emit remoteTidChanged(tid);
            emit logMessage(QString("[AutoConn] 已连接，对端 TID = %1").arg(tid));
        }
    } else {
        // 无在线终端
        if (m_state == ConnState::CONNECTED) {
            // 刚断线
            emit logMessage("[AutoConn] 检测到连接断开，切换重连模式");
            m_remoteTid = 0;
            emit remoteTidChanged(0);
            doReconnect();
        } else if (m_state == ConnState::RECONNECTING || m_state == ConnState::CONNECTING) {
            // 仍在重连中，不重复操作，等定时器
        }
        // DISCONNECTED 时地面端已有 retryTimer 在跑，UAV端静等即可
    }
}

void AutoConnController::onTerminalListUpdated()
{
    // 终端列表变化时立即同步一次状态（不等心跳定时器），消除最长 1 秒延迟
    bool connected = isConnected();
    emit logMessage(QString("[AutoConn] 终端列表更新，当前在线: %1")
                        .arg(connected ? "是" : "否"));
    onHeartbeatTimer();  // 立即执行一次心跳逻辑，实时更新连接状态
}

// ============================================================
//  private helpers
// ============================================================

void AutoConnController::readConfig()
{
    // 从已初始化的 ini 中读取
    int role = GetIntegerKeyIni("AutoConn", "Role", (int)NodeRole::UAV);
    m_role = (role == 2) ? NodeRole::GROUND : NodeRole::UAV;

    m_localAddr  = GetStringValueKeyIni("AutoConn", "LocalIP",  "127.0.0.1");
    m_localPort  = GetIntegerKeyIni("AutoConn", "LocalPort", 3020);
    m_remoteAddr = GetStringValueKeyIni("AutoConn", "RemoteIP", "127.0.0.1");
    m_remotePort = GetIntegerKeyIni("AutoConn", "RemotePort", 3021);
    m_remoteTid  = (unsigned int)GetIntegerKeyIni("AutoConn", "RemoteTID", 102);

    m_retryIntervalMs    = GetIntegerKeyIni("AutoConn", "RetryIntervalMs",   3000);
    m_heartbeatTimeoutMs = GetIntegerKeyIni("AutoConn", "HeartbeatTimeoutMs", 5000);
}

void AutoConnController::setState(ConnState s)
{
    if (m_state == s) return;
    m_state = s;
    emit stateChanged(s);
}

void AutoConnController::doStartListening()
{
    if (m_serverCreated) return;

    emit logMessage(QString("[AutoConn] UAV端 开始监听 %1:%2")
                        .arg(QString::fromStdString(m_localAddr))
                        .arg(m_localPort));

    bool ok = createServer(m_localAddr, m_localPort, false);
    if (ok) {
        m_serverCreated = true;
        setState(ConnState::CONNECTING);  // 监听中，等待对端接入
        emit logMessage("[AutoConn] UAV端 监听已建立，等待地面端接入...");
    } else {
        emit logMessage("[AutoConn] UAV端 创建监听失败，将在下次心跳重试");
        // 5 秒后在心跳里重试（不使用额外定时器，由心跳驱动）
    }
}

void AutoConnController::doConnect()
{
    // 地面端发起主动连接
    bool ok = createClient(m_localAddr, m_localPort,
                           m_remoteAddr, (unsigned int)m_remotePort,
                           false);
    if (!ok) {
        emit logMessage(QString("[AutoConn] 连接 %1:%2 失败，等待重试")
                            .arg(QString::fromStdString(m_remoteAddr))
                            .arg(m_remotePort));
    }
}

void AutoConnController::doReconnect()
{
    setState(ConnState::RECONNECTING);

    if (m_role == NodeRole::UAV) {
        // UAV端：重新启动监听（若之前监听 socket 已关闭则重建）
        m_serverCreated = false;
        doStartListening();
    } else {
        // 地面端：重启重试定时器
        if (!m_retryTimer->isActive()) {
            m_retryTimer->start(m_retryIntervalMs);
        }
        // 立即尝试一次
        doConnect();
    }
}

bool AutoConnController::isConnected() const
{
    MutexLockGuard guard(_terminal_Mutex);
    for (const auto &c : _clientTemplate) {
        if (c && c->_bRunning) {
            return true;
        }
    }
    return false;
}

unsigned int AutoConnController::getActiveTid() const
{
    MutexLockGuard guard(_terminal_Mutex);
    for (const auto &c : _clientTemplate) {
        if (c && c->_bRunning) {
            return c->_TID;
        }
    }
    return 0;
}

// ============================================================
//  速率轮询
// ============================================================

void AutoConnController::onRateTimer()
{
    if (m_stopped) return;

    float sendKbps = 0.0f;
    float recvKbps = 0.0f;

    auto sendBw = BigDataTransfer_GetAllFileSendBandWidth();
    if (sendBw && sendBw->m_status == 200) {
        sendKbps = sendBw->m_total_rate;
    }

    auto recvBw = BigDataTransfer_GetAllFileRecvBandWidth();
    if (recvBw && recvBw->m_status == 200) {
        recvKbps = recvBw->m_total_rate;
    }

    emit transferRateUpdated(sendKbps, recvKbps);
}

// ============================================================
//  发送文件
// ============================================================

bool AutoConnController::sendFile(const std::string &filePath, const std::string &fileName)
{
    unsigned int tid = getActiveTid();
    if (tid == 0) {
        emit logMessage("[Transfer] 未连接，无法发送文件");
        return false;
    }
    return createTransferSendTask(tid, fileName, filePath);
}
