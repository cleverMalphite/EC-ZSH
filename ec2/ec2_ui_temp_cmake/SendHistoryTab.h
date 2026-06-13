#ifndef SENDHISTORYTAB_H
#define SENDHISTORYTAB_H

#include <QWidget>

class QTableWidget;

/**
 * SendHistoryTab - 发送历史表格视图
 *
 * 展示文件发送任务记录：任务号、接收端 TID、文件名、文件大小、状态、平均发送速率、完成时间。
 */
class SendHistoryTab : public QWidget
{
    Q_OBJECT

public:
    explicit SendHistoryTab(QWidget *parent = nullptr);

    void addSendRecord(unsigned int taskId,
                       unsigned int receiverTid,
                       const QString &fileName,
                       quint64 fileSize,
                       const QString &status,
                       float avgSpeedKbps,
                       const QString &finishTime);

    void upsertSendProgress(unsigned int taskId,
                            unsigned int receiverTid,
                            const QString &fileName,
                            quint64 fileSize,
                            const QString &status,
                            float rateKbps,
                            const QString &finishTime);

    void updateTaskStatus(unsigned int taskId, const QString &newStatus);
    void clearRecords();

private:
    QTableWidget *m_table = nullptr;

    void setupUI();
    int findRowByTaskId(unsigned int taskId) const;
    static QString formatRate(float kbps);
    static QString formatFileSize(quint64 bytes);
    static void applyStatusColor(class QTableWidgetItem *statusItem, const QString &status);
};

#endif // SENDHISTORYTAB_H
