#ifndef RECEIVEHISTORYTAB_H
#define RECEIVEHISTORYTAB_H

#include <QWidget>

class QTableWidget;

/**
 * ReceiveHistoryTab - 接收历史表格视图
 *
 * 展示文件接收任务记录：任务号、发送端 TID、文件名、接收状态、平均接收速率、接收时间。
 */
class ReceiveHistoryTab : public QWidget
{
    Q_OBJECT

public:
    explicit ReceiveHistoryTab(QWidget *parent = nullptr);

    void addRecvRecord(unsigned int taskId,
                       unsigned int senderTid,
                       const QString &fileName,
                       const QString &status,
                       float avgSpeedKbps,
                       const QString &recvTime);

    void upsertRecvProgress(unsigned int taskId,
                            unsigned int senderTid,
                            const QString &fileName,
                            const QString &status,
                            float rateKbps,
                            const QString &recvTime);

    void updateRecvStatus(unsigned int taskId, const QString &newStatus);
    void clearRecords();

private:
    QTableWidget *m_table = nullptr;

    void setupUI();
    int findRowByTaskId(unsigned int taskId) const;
    static QString formatRate(float kbps);
    static void applyStatusColor(class QTableWidgetItem *statusItem, const QString &status);
};

#endif // RECEIVEHISTORYTAB_H
