#include "AutoConnController.h"
#include "function.h"

#include <QDateTime>
#include <QDebug>
#include <QString>
#include "../BigDataTransfer/BigDataTransferApi.h"

#include <arpa/inet.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

// 引用 function.cpp 里的全局终端列表和任务状态
extern std::vector<std::shared_ptr<ClientTemplate>> _clientTemplate;
extern std::vector<std::shared_ptr<SendTaskState>>  _sendTaskState;
extern std::vector<std::shared_ptr<RecvTaskState>>  _recvTaskState;
extern pthread_mutex_t _terminal_Mutex;
extern pthread_mutex_t _sendTask_Mutex;
extern pthread_mutex_t _recvTask_Mutex;
extern DataUpdater *dataUpdater;

namespace {

std::string trim_copy(const std::string &input)
{
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }

    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }

    return input.substr(start, end - start);
}

std::vector<std::string> split_copy(const std::string &input, char delim)
{
    std::vector<std::string> items;
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, delim)) {
        items.push_back(trim_copy(token));
    }
    return items;
}

bool parse_positive_int(const std::string &text, int &value)
{
    if (text.empty()) {
        return false;
    }

    char *end = nullptr;
    errno = 0;
    long parsed = std::strtol(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || *end != '\0' || parsed <= 0 || parsed > INT_MAX) {
        return false;
    }

    value = static_cast<int>(parsed);
    return true;
}

bool parse_hello_message(const std::string &msg,
                         unsigned int &tid,
                         int &tcpPort,
                         unsigned int &targetTid)
{
    int matched = std::sscanf(msg.c_str(), "DISCOVER_HELLO tid=%u tcp=%d target=%u",
                              &tid, &tcpPort, &targetTid);
    return matched == 3 && tid > 0 && tcpPort > 0;
}

bool parse_ack_message(const std::string &msg,
                       unsigned int &tid,
                       int &tcpPort)
{
    int matched = std::sscanf(msg.c_str(), "DISCOVER_ACK tid=%u tcp=%d", &tid, &tcpPort);
    return matched == 2 && tid > 0 && tcpPort > 0;
}

} // namespace

// ============================================================
//  构造 / 析构
// ============================================================

AutoConnController::AutoConnController(QObject *parent)
    : QObject(parent)
{
    m_retryTimer     = new QTimer(this);
    m_heartbeatTimer = new QTimer(this);
    m_rateTimer      = new QTimer(this);
    m_discoveryTimer = new QTimer(this);

    m_retryTimer->setSingleShot(false);
    m_heartbeatTimer->setSingleShot(false);
    m_rateTimer->setSingleShot(false);
    m_discoveryTimer->setSingleShot(false);

    connect(m_retryTimer,     &QTimer::timeout, this, &AutoConnController::onRetryTimer);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &AutoConnController::onHeartbeatTimer);
    connect(m_rateTimer,      &QTimer::timeout, this, &AutoConnController::onRateTimer);
    connect(m_discoveryTimer, &QTimer::timeout, this, &AutoConnController::onDiscoveryTimer);
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
    m_stopped = false;

    readConfig();

    if (m_enableDiscovery) {
        if (initDiscoverySocket()) {
            m_discoveryTimer->start(m_discoveryIntervalMs);
            emit logMessage(QString("[Discovery] 已启用，端口=%1，固定候选=%2")
                                .arg(m_discoveryPort)
                                .arg(static_cast<int>(m_discoveryPeers.size())));
        } else {
            emit logMessage("[Discovery] 初始化失败，回退固定 RemoteIP 重连模式");
            m_enableDiscovery = false;
        }
    }

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
                            emit sendProgressUpdated(task_id, s->_TID, name, kbps, pct);
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
                            float kbps = r->_transferSpeed;
                            if (kbps <= 0.0f) {
                                auto recvBw = BigDataTransfer_GetAllFileRecvBandWidth();
                                if (recvBw && recvBw->m_status == 200) {
                                    kbps = recvBw->m_total_rate;
                                }
                            }
                            QString name = QString::fromStdString(r->_taskName);
                            emit recvProgressUpdated(task_id, r->_TID, name, kbps, r->_process);
                            emit logMessage(QString("[Transfer] 接收任务#%1 %2  %3%  %4 kbps")
                                .arg(task_id)
                                .arg(name)
                                .arg(r->_process)
                                .arg(kbps, 0, 'f', 1));
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
    if (m_discoveryTimer) { m_discoveryTimer->stop(); }
    closeDiscoverySocket();
}

void AutoConnController::requestReconnect()
{
    emit logMessage("[AutoConn] 手动触发重连");
    doReconnect();
}

void AutoConnController::applyManualConnection(const QString &localAddr,
                                               int localPort,
                                               const QString &remoteAddr,
                                               int remotePort)
{
    const QString local = localAddr.trimmed();
    const QString remote = remoteAddr.trimmed();

    if (!local.isEmpty()) {
        m_localAddr = local.toStdString();
    }
    if (!remote.isEmpty()) {
        m_remoteAddr = remote.toStdString();
    }
    if (localPort >= 0 && localPort <= 65535) {
        m_localPort = localPort;
    }
    if (remotePort > 0 && remotePort <= 65535) {
        m_remotePort = remotePort;
    }

    m_discoveredRemoteAddr.clear();
    m_discoveredRemotePort = 0;
    m_discoveredRemoteTid = 0;
    m_discoveredRemoteSeenMs = 0;

    emit logMessage(QString("[AutoConn] 手动连接参数已更新: 本地 %1:%2  远端 %3:%4")
                        .arg(QString::fromStdString(m_localAddr))
                        .arg(m_localPort)
                        .arg(QString::fromStdString(m_remoteAddr))
                        .arg(m_remotePort));

    requestReconnect();
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
    std::string targetAddr;
    int targetPort = 0;
    resolveConnectTarget(targetAddr, targetPort);
    emit logMessage(QString("[AutoConn] 重试连接 → %1:%2")
                        .arg(QString::fromStdString(targetAddr))
                        .arg(targetPort));
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

    const QStringList onlineTids = collectOnlineTidList();
    if (onlineTids != m_lastOnlineTidList) {
        m_lastOnlineTidList = onlineTids;
        emit terminalListChanged(onlineTids);
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

void AutoConnController::onDiscoveryTimer()
{
    if (m_stopped || !m_enableDiscovery) {
        return;
    }

    sendDiscoveryHello();
    drainDiscoveryPackets();

    if (m_role != NodeRole::GROUND || isConnected()) {
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const bool recentAck = !m_discoveredRemoteAddr.empty()
                           && (nowMs - m_discoveredRemoteSeenMs <= m_discoveryTimeoutMs);
    if (!recentAck) {
        return;
    }

    if (nowMs - m_lastDiscoveryConnectAttemptMs < 1000) {
        return;
    }

    m_lastDiscoveryConnectAttemptMs = nowMs;
    if (m_state == ConnState::DISCONNECTED) {
        setState(ConnState::CONNECTING);
    }

    doConnect();
}

// ============================================================
//  private helpers
// ============================================================

void AutoConnController::readConfig()
{
    // 从已初始化的 ini 中读取
    int role = GetIntegerKeyIni("AutoConn", "Role", (int)NodeRole::UAV);
    m_role = (role == 2) ? NodeRole::GROUND : NodeRole::UAV;
    m_localTid = static_cast<unsigned int>(GetIntegerKeyIni("Main", "DeviceID", 0));

    m_localAddr  = GetStringValueKeyIni("AutoConn", "LocalIP",  "127.0.0.1");
    m_localPort  = GetIntegerKeyIni("AutoConn", "LocalPort", 3020);
    m_remoteAddr = GetStringValueKeyIni("AutoConn", "RemoteIP", "127.0.0.1");
    m_remotePort = GetIntegerKeyIni("AutoConn", "RemotePort", 3021);
    m_remoteTid  = (unsigned int)GetIntegerKeyIni("AutoConn", "RemoteTID", 102);

    m_retryIntervalMs    = GetIntegerKeyIni("AutoConn", "RetryIntervalMs",   3000);
    m_heartbeatTimeoutMs = GetIntegerKeyIni("AutoConn", "HeartbeatTimeoutMs", 5000);

    m_discoveredRemoteAddr.clear();
    m_discoveredRemotePort = 0;
    m_discoveredRemoteTid = 0;
    m_discoveredRemoteSeenMs = 0;
    m_lastDiscoveryConnectAttemptMs = 0;

    m_enableDiscovery     = GetBoolValueKeyIni("AutoConn", "EnableDiscovery", false);
    m_discoveryPort       = GetIntegerKeyIni("AutoConn", "DiscoveryPort", 39001);
    m_discoveryIntervalMs = GetIntegerKeyIni("AutoConn", "DiscoveryIntervalMs", 1000);
    m_discoveryTimeoutMs  = GetIntegerKeyIni("AutoConn", "DiscoveryTimeoutMs", 5000);
    if (m_discoveryPort <= 0) {
        m_discoveryPort = 39001;
    }
    if (m_discoveryIntervalMs <= 0) {
        m_discoveryIntervalMs = 1000;
    }
    if (m_discoveryTimeoutMs <= 0) {
        m_discoveryTimeoutMs = 5000;
    }

    m_discoveryPeers.clear();
    const std::string peersText = GetStringValueKeyIni("AutoConn", "DiscoveryPeers", "");
    if (!peersText.empty()) {
        const auto entries = split_copy(peersText, ',');
        for (const auto &entry : entries) {
            if (entry.empty()) {
                continue;
            }
            const auto parts = split_copy(entry, ':');
            if (parts.size() < 2) {
                continue;
            }

            int tid = 0;
            if (!parse_positive_int(parts[0], tid)) {
                continue;
            }

            DiscoveryPeerCfg peer;
            peer.tid = static_cast<unsigned int>(tid);
            peer.ip = parts[1];
            peer.tcpPort = m_remotePort;

            if (parts.size() >= 3) {
                int parsedPort = 0;
                if (parse_positive_int(parts[2], parsedPort)) {
                    peer.tcpPort = parsedPort;
                }
            }

            if (peer.ip.empty() || peer.tid == 0) {
                continue;
            }
            m_discoveryPeers.push_back(peer);
        }
    }

    // 未配置 DiscoveryPeers 时，至少保留 RemoteTID/RemoteIP 作为一个探测目标。
    if (m_discoveryPeers.empty() && m_remoteTid > 0 && !m_remoteAddr.empty()) {
        DiscoveryPeerCfg fallback;
        fallback.tid = m_remoteTid;
        fallback.ip = m_remoteAddr;
        fallback.tcpPort = m_remotePort;
        m_discoveryPeers.push_back(fallback);
    }
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
    std::string targetAddr;
    int targetPort = 0;
    const bool useDiscovered = resolveConnectTarget(targetAddr, targetPort);

    if (useDiscovered) {
        emit logMessage(QString("[Discovery] 使用探测目标 %1:%2 (TID=%3)")
                            .arg(QString::fromStdString(targetAddr))
                            .arg(targetPort)
                            .arg(m_discoveredRemoteTid));
    }

    // 地面端发起主动连接
    bool ok = createClient(m_localAddr, m_localPort,
                           targetAddr, (unsigned int)targetPort,
                           false);
    if (!ok) {
        emit logMessage(QString("[AutoConn] 连接 %1:%2 失败，等待重试")
                            .arg(QString::fromStdString(targetAddr))
                            .arg(targetPort));
    }
}

bool AutoConnController::resolveConnectTarget(std::string &addr, int &port) const
{
    if (m_enableDiscovery && !m_discoveredRemoteAddr.empty()) {
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        if (nowMs - m_discoveredRemoteSeenMs <= m_discoveryTimeoutMs) {
            addr = m_discoveredRemoteAddr;
            port = (m_discoveredRemotePort > 0) ? m_discoveredRemotePort : m_remotePort;
            return true;
        }
    }

    addr = m_remoteAddr;
    port = m_remotePort;
    return false;
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

QStringList AutoConnController::collectOnlineTidList() const
{
    QStringList tids;
    MutexLockGuard guard(_terminal_Mutex);
    for (const auto &c : _clientTemplate) {
        if (c && c->_bRunning && c->_TID > 0) {
            const QString tidStr = QString::number(c->_TID);
            if (!tids.contains(tidStr)) {
                tids.push_back(tidStr);
            }
        }
    }
    return tids;
}

bool AutoConnController::initDiscoverySocket()
{
    closeDiscoverySocket();

    m_discoverySockFd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (m_discoverySockFd < 0) {
        return false;
    }

    int one = 1;
    setsockopt(m_discoverySockFd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    sockaddr_in localAddr;
    std::memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(static_cast<unsigned short>(m_discoveryPort));
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (::bind(m_discoverySockFd,
               reinterpret_cast<sockaddr *>(&localAddr),
               sizeof(localAddr)) != 0) {
        closeDiscoverySocket();
        return false;
    }

    int flags = fcntl(m_discoverySockFd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(m_discoverySockFd, F_SETFL, flags | O_NONBLOCK);
    }

    return true;
}

void AutoConnController::closeDiscoverySocket()
{
    if (m_discoverySockFd >= 0) {
        ::close(m_discoverySockFd);
        m_discoverySockFd = -1;
    }
}

void AutoConnController::sendDiscoveryHello()
{
    if (m_discoverySockFd < 0 || m_discoveryPeers.empty()) {
        return;
    }

    for (const auto &peer : m_discoveryPeers) {
        if (peer.ip.empty()) {
            continue;
        }
        if (peer.tid == m_localTid) {
            continue;
        }
        if (m_remoteTid > 0 && peer.tid != m_remoteTid) {
            continue;
        }

        sockaddr_in peerAddr;
        std::memset(&peerAddr, 0, sizeof(peerAddr));
        peerAddr.sin_family = AF_INET;
        peerAddr.sin_port = htons(static_cast<unsigned short>(m_discoveryPort));
        if (inet_pton(AF_INET, peer.ip.c_str(), &peerAddr.sin_addr) != 1) {
            continue;
        }

        char buffer[160] = {0};
        std::snprintf(buffer, sizeof(buffer),
                      "DISCOVER_HELLO tid=%u tcp=%d target=%u",
                      m_localTid, m_localPort, m_remoteTid);

        ::sendto(m_discoverySockFd,
                 buffer,
                 std::strlen(buffer),
                 0,
                 reinterpret_cast<sockaddr *>(&peerAddr),
                 sizeof(peerAddr));
    }
}

void AutoConnController::drainDiscoveryPackets()
{
    if (m_discoverySockFd < 0) {
        return;
    }

    for (;;) {
        char buffer[512] = {0};
        sockaddr_in peerAddr;
        std::memset(&peerAddr, 0, sizeof(peerAddr));
        socklen_t addrLen = sizeof(peerAddr);

        const ssize_t n = ::recvfrom(m_discoverySockFd,
                                     buffer,
                                     sizeof(buffer) - 1,
                                     0,
                                     reinterpret_cast<sockaddr *>(&peerAddr),
                                     &addrLen);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            break;
        }

        buffer[n] = '\0';
        char ipBuf[INET_ADDRSTRLEN] = {0};
        const char *ip = inet_ntop(AF_INET, &peerAddr.sin_addr, ipBuf, sizeof(ipBuf));
        if (ip == nullptr) {
            continue;
        }

        handleDiscoveryPacket(buffer, ipBuf, ntohs(peerAddr.sin_port));
    }
}

void AutoConnController::handleDiscoveryPacket(const std::string &payload,
                                               const std::string &senderIp,
                                               unsigned short senderPort)
{
    unsigned int tid = 0;
    int tcpPort = 0;
    unsigned int targetTid = 0;

    if (parse_hello_message(payload, tid, tcpPort, targetTid)) {
        if (tid == 0 || tid == m_localTid) {
            return;
        }
        if (targetTid != 0 && m_localTid != 0 && targetTid != m_localTid) {
            return;
        }

        char ack[128] = {0};
        std::snprintf(ack, sizeof(ack), "DISCOVER_ACK tid=%u tcp=%d", m_localTid, m_localPort);

        sockaddr_in ackAddr;
        std::memset(&ackAddr, 0, sizeof(ackAddr));
        ackAddr.sin_family = AF_INET;
        ackAddr.sin_port = htons(senderPort);
        if (inet_pton(AF_INET, senderIp.c_str(), &ackAddr.sin_addr) == 1) {
            ::sendto(m_discoverySockFd,
                     ack,
                     std::strlen(ack),
                     0,
                     reinterpret_cast<sockaddr *>(&ackAddr),
                     sizeof(ackAddr));
        }
        return;
    }

    if (parse_ack_message(payload, tid, tcpPort)) {
        onDiscoveryAck(tid, tcpPort, senderIp);
    }
}

void AutoConnController::onDiscoveryAck(unsigned int tid,
                                        int tcpPort,
                                        const std::string &senderIp)
{
    if (tid == 0 || senderIp.empty()) {
        return;
    }
    if (m_localTid != 0 && tid == m_localTid) {
        return;
    }
    if (m_remoteTid > 0 && tid != m_remoteTid) {
        return;
    }

    const bool changed = (m_discoveredRemoteAddr != senderIp)
                         || (m_discoveredRemotePort != tcpPort)
                         || (m_discoveredRemoteTid != tid);

    m_discoveredRemoteAddr = senderIp;
    m_discoveredRemotePort = tcpPort;
    m_discoveredRemoteTid = tid;
    m_discoveredRemoteSeenMs = QDateTime::currentMSecsSinceEpoch();

    if (changed) {
        emit logMessage(QString("[Discovery] 发现对端 TID=%1 %2:%3")
                            .arg(tid)
                            .arg(QString::fromStdString(senderIp))
                            .arg(tcpPort));
    }
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

    sendKbps = smoothRate(m_sendRateSamples, sendKbps);
    recvKbps = smoothRate(m_recvRateSamples, recvKbps);

    emit transferRateUpdated(sendKbps, recvKbps);
}

float AutoConnController::smoothRate(std::deque<float> &samples, float value)
{
    samples.push_back(value);
    while (samples.size() > kRateSmoothSampleCount) {
        samples.pop_front();
    }

    float total = 0.0f;
    for (float sample : samples) {
        total += sample;
    }
    return samples.empty() ? 0.0f : (total / static_cast<float>(samples.size()));
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

bool AutoConnController::sendFileToTid(unsigned int tid,
                                       const std::string &filePath,
                                       const std::string &fileName)
{
    if (tid == 0) {
        emit logMessage("[Transfer] 目标终端无效，无法发送文件");
        return false;
    }
    return createTransferSendTask(tid, fileName, filePath);
}
