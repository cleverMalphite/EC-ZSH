#ifndef AUTOCONNCONTROLLER_H
#define AUTOCONNCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QStringList>
#include <string>
#include <memory>
#include <deque>
#include <vector>

enum class NodeRole {
    UAV    = 1,
    GROUND = 2
};

enum class ConnState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING
};

class AutoConnController : public QObject
{
    Q_OBJECT

public:
    explicit AutoConnController(QObject *parent = nullptr);
    ~AutoConnController();

    void start();
    void stop();

    ConnState currentState() const { return m_state; }
    NodeRole  role()         const { return m_role; }

    void requestReconnect();
    void applyManualConnection(const QString &localAddr, int localPort,
                               const QString &remoteAddr, int remotePort);

    QString localAddr() const { return QString::fromStdString(m_localAddr); }
    int localPort() const { return m_localPort; }
    QString remoteAddr() const { return QString::fromStdString(m_remoteAddr); }
    int remotePort() const { return m_remotePort; }

signals:
    void stateChanged(ConnState newState);
    void logMessage(const QString &msg);
    void remoteTidChanged(unsigned int tid);
    void terminalListChanged(const QStringList &tids);

    void transferRateUpdated(float sendKbps, float recvKbps);

    void sendProgressUpdated(unsigned int taskId, unsigned int receiverTid,
                             const QString &filename, float rateKbps, unsigned int percent);

    void recvProgressUpdated(unsigned int taskId, unsigned int senderTid,
                             const QString &filename, float rateKbps, unsigned int percent);

private slots:
    void onRetryTimer();
    void onHeartbeatTimer();
    void onTerminalListUpdated();
    void onRateTimer();
    void onDiscoveryTimer();

public:
    bool sendFile(const std::string &filePath, const std::string &fileName);
    bool sendFileToTid(unsigned int tid, const std::string &filePath, const std::string &fileName);

private:
    struct DiscoveryPeerCfg {
        unsigned int tid = 0;
        std::string ip;
        int tcpPort = 0;
    };

    NodeRole     m_role           = NodeRole::UAV;
    unsigned int m_localTid       = 0;
    std::string  m_localAddr;
    int          m_localPort      = 3020;
    std::string  m_remoteAddr;
    int          m_remotePort     = 3021;
    unsigned int m_remoteTid      = 0;

    int          m_retryIntervalMs  = 3000;
    int          m_heartbeatTimeoutMs = 5000;

    bool         m_enableDiscovery      = false;
    int          m_discoveryPort        = 39001;
    int          m_discoveryIntervalMs  = 1000;
    int          m_discoveryTimeoutMs   = 5000;
    std::vector<DiscoveryPeerCfg> m_discoveryPeers;

    ConnState    m_state          = ConnState::DISCONNECTED;
    bool         m_serverCreated  = false;
    bool         m_stopped        = false;

    qint64       m_lastSeenMs     = 0;

    int          m_discoverySockFd = -1;
    std::string  m_discoveredRemoteAddr;
    int          m_discoveredRemotePort = 0;
    unsigned int m_discoveredRemoteTid = 0;
    qint64       m_discoveredRemoteSeenMs = 0;
    qint64       m_lastDiscoveryConnectAttemptMs = 0;

    QTimer      *m_retryTimer     = nullptr;
    QTimer      *m_heartbeatTimer = nullptr;
    QTimer      *m_rateTimer      = nullptr;
    QTimer      *m_discoveryTimer = nullptr;

    void readConfig();
    void setState(ConnState s);
    void doStartListening();
    void doConnect();
    bool resolveConnectTarget(std::string &addr, int &port) const;
    void doReconnect();

    bool initDiscoverySocket();
    void closeDiscoverySocket();
    void sendDiscoveryHello();
    void drainDiscoveryPackets();
    void handleDiscoveryPacket(const std::string &payload,
                               const std::string &senderIp,
                               unsigned short senderPort);
    void onDiscoveryAck(unsigned int tid,
                        int tcpPort,
                        const std::string &senderIp);

    bool isConnected() const;
    unsigned int getActiveTid() const;
    QStringList collectOnlineTidList() const;
    float smoothRate(std::deque<float> &samples, float value);

    std::deque<float> m_sendRateSamples;
    std::deque<float> m_recvRateSamples;
    QStringList m_lastOnlineTidList;
    static constexpr std::size_t kRateSmoothSampleCount = 3;
};

#endif // AUTOCONNCONTROLLER_H
