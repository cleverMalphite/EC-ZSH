#ifndef AUTOCONNWINDOW_H
#define AUTOCONNWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QDateTime>
#include <QFileDialog>
#include "AutoConnController.h"

namespace Ui {
class AutoConnWindow;
}

/**
 * AutoConnWindow
 *
 * 纯显示窗口：只渲染连接状态，不含任何手工操作的业务逻辑。
 * 状态驱动：AutoConnController 发出 stateChanged 信号 → 本窗口更新 UI。
 *
 * 四种状态显示：
 *   DISCONNECTED  → 灰色  ●  未连接
 *   CONNECTING    → 蓝色  ●  连接中...
 *   CONNECTED     → 绿色  ●  已连接
 *   RECONNECTING  → 橙色  ●  重连中...
 */
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
    void onClockTick();
    void onReconnectBtnClicked();
    void onBrowseBtnClicked();
    void onSendFileBtnClicked();
    void onTransferRateUpdated(float sendKbps, float recvKbps);
    void onSendProgressUpdated(unsigned int taskId, const QString &filename,
                               float rateKbps, unsigned int percent);

private:
    Ui::AutoConnWindow *ui;
    AutoConnController *m_ctrl = nullptr;

    // 连接成功后开始计时
    QDateTime  m_connectedAt;
    bool       m_timingConn = false;
    QTimer    *m_clockTimer = nullptr;

    QString    m_selectedFilePath;   // 浏览选中的文件完整路径

    void applyStateStyle(ConnState s);
    void updateUptimeLabel();
};

#endif // AUTOCONNWINDOW_H
