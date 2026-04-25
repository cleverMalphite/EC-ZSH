#ifndef AUTOCONNCONTROLLER_H
#define AUTOCONNCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <string>
#include <memory>

/**
 * 终端角色枚举
 *   UAV  : 无人机端 —— 开机后自动监听，被动等待地面端连接
 *   GROUND: 地面端  —— 开软件后自动重连，主动发起连接
 */
enum class NodeRole {
    UAV    = 1,
    GROUND = 2
};

/**
 * 连接状态枚举（对应 UI 四种显示）
 */
enum class ConnState {
    DISCONNECTED,   // 未连接
    CONNECTING,     // 连接中（地面端正在主动发起）
    CONNECTED,      // 已连接
    RECONNECTING    // 重连中（检测到断线后）
};

/**
 * AutoConnController
 *
 * 职责：
 *   - 读取配置文件，判断本端角色（UAV / GROUND）
 *   - UAV端：启动后立即调用 createServer() 开始监听，等待终端列表回调确认连接
 *   - 地面端：启动后循环调用 createClient()，连接失败则按重连间隔重试
 *   - 监控已连接终端心跳，断线后切换到 RECONNECTING 并重新发起
 *   - 通过 Qt 信号把状态变化推送给 UI
 *
 * 线程安全：
 *   所有公共槽和状态变更均在 Qt 主线程（QTimer 回调）中执行，无须额外加锁
 */
class AutoConnController : public QObject
{
    Q_OBJECT

public:
    explicit AutoConnController(QObject *parent = nullptr);
    ~AutoConnController();

    /** 启动控制器（在 QApplication::exec() 前调用） */
    void start();

    /** 停止控制器，释放资源 */
    void stop();

    ConnState currentState() const { return m_state; }
    NodeRole  role()         const { return m_role; }

    /** 供外部强制触发一次重连（可选，UI 按钮调用） */
    void requestReconnect();

signals:
    /** 连接状态发生变化时发出 */
    void stateChanged(ConnState newState);

    /** 补充文字说明（调试用，可显示在 UI 日志区） */
    void logMessage(const QString &msg);

    /** 终端列表变化时发出（携带远端 TID，0 表示列表为空） */
    void remoteTidChanged(unsigned int tid);

    /**
     * 实时传输速率更新（每秒一次）
     *   sendRate / recvRate 单位：kbps，状态码 200 = 有效
     */
    void transferRateUpdated(float sendKbps, float recvKbps);

    /**
     * 文件发送进度更新
     *   taskId   全局任务号
     *   filename 文件名
     *   rateKbps 实时速率 kbps
     *   percent  进度 [0,100]
     */
    void sendProgressUpdated(unsigned int taskId, const QString &filename,
                             float rateKbps, unsigned int percent);

private slots:
    /** 地面端重连定时器触发 */
    void onRetryTimer();

    /** 心跳检测定时器触发（双端均需要） */
    void onHeartbeatTimer();

    /** 内部：终端列表更新通知（由底层全局回调 -> Qt 信号 -> 此槽） */
    void onTerminalListUpdated();

    /** 速率轮询定时器：每秒查询一次 BigDataTransfer 带宽 */
    void onRateTimer();

public:
    /** 发起文件发送任务（供 UI 按钮调用） */
    bool sendFile(const std::string &filePath, const std::string &fileName);

private:
    // -------- 配置 --------
    NodeRole     m_role           = NodeRole::UAV;
    std::string  m_localAddr;
    int          m_localPort      = 3020;
    std::string  m_remoteAddr;
    int          m_remotePort     = 3021;
    unsigned int m_remoteTid      = 0;

    // 地面端重连间隔（ms），可从 ini 读取
    int          m_retryIntervalMs  = 3000;
    // 心跳超时（ms）：超过此时间无终端则认为断线
    int          m_heartbeatTimeoutMs = 5000;

    // -------- 状态 --------
    ConnState    m_state          = ConnState::DISCONNECTED;
    bool         m_serverCreated  = false;  // UAV端是否已建立监听
    bool         m_stopped        = false;

    // 上次收到终端在线确认的时间戳（毫秒，用于心跳超时检测）
    qint64       m_lastSeenMs     = 0;

    // -------- 定时器 --------
    QTimer      *m_retryTimer     = nullptr;   // 地面端重连用
    QTimer      *m_heartbeatTimer = nullptr;   // 心跳监测用
    QTimer      *m_rateTimer      = nullptr;   // 速率轮询用

    // -------- 内部辅助 --------
    void readConfig();
    void setState(ConnState s);
    void doStartListening();        // UAV端：创建服务器监听
    void doConnect();               // 地面端：发起连接
    void doReconnect();             // 断线后重新发起（UAV/GROUND 分支）

    bool isConnected() const;       // 检查 _clientTemplate 里是否有在线终端
    unsigned int getActiveTid() const; // 取第一个在线终端 TID
};

#endif // AUTOCONNCONTROLLER_H
