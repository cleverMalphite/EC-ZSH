#ifndef AUTOCONNWINDOW_H
#define AUTOCONNWINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QTimer>
#include <QDateTime>
#include <QFileDialog>
#include <QStringList>
#include "AutoConnController.h"

class SatelliteImageWidget;
class SendHistoryTab;
class ReceiveHistoryTab;
class RealtimeStreamDialog;

namespace Ui {
class AutoConnWindow;
}

class AutoConnWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AutoConnWindow(AutoConnController *ctrl, QWidget *parent = nullptr);
    ~AutoConnWindow();

public slots:
    void cleanUp();

private slots:
    void onStateChanged(ConnState s);
    void onLogMessage(const QString &msg);
    void onRemoteTidChanged(unsigned int tid);
    void onTerminalListChanged(const QStringList &tids);
    void onClockTick();
    void onReconnectBtnClicked();
    void onRealtimeBtnClicked();
    void onBrowseBtnClicked();
    void onSendFileBtnClicked();
    void onTransferRateUpdated(float sendKbps, float recvKbps);
    void onSendProgressUpdated(unsigned int taskId, unsigned int receiverTid,
                               const QString &filename, float rateKbps, unsigned int percent);
    void onRecvProgressUpdated(unsigned int taskId, unsigned int senderTid,
                               const QString &filename, float rateKbps, unsigned int percent);

private:
    Ui::AutoConnWindow *ui;
    AutoConnController *m_ctrl = nullptr;

    SatelliteImageWidget *m_imageWidget     = nullptr;
    QPlainTextEdit       *m_logEdit         = nullptr;
    SendHistoryTab       *m_sendHistoryTab  = nullptr;
    ReceiveHistoryTab    *m_recvHistoryTab  = nullptr;

    QDateTime  m_connectedAt;
    bool       m_timingConn = false;
    QTimer    *m_clockTimer = nullptr;

    RealtimeStreamDialog *m_realtimeDlg = nullptr;
    QString    m_selectedFilePath;

    float      m_lastSendRateKbps = 0.0f;
    float      m_lastRecvRateKbps = 0.0f;
    qint64     m_lastSendRateMs   = 0;
    qint64     m_lastRecvRateMs   = 0;

    void setupCustomWidgets();
    void applyStateStyle(ConnState s);
    void updateUptimeLabel();
    unsigned int currentRemoteTid() const;
    unsigned int selectedTargetTid() const;
    void syncHistoryFromLogLine(const QString &msg);
    void updateSendButtonEnabled();
};

#endif // AUTOCONNWINDOW_H
