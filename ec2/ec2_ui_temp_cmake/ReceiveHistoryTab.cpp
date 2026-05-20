#include "ReceiveHistoryTab.h"

#include <QColor>
#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

ReceiveHistoryTab::ReceiveHistoryTab(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void ReceiveHistoryTab::setupUI()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(0);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("任务号"),
        QStringLiteral("发送端号 (TID)"),
        QStringLiteral("文件名"),
        QStringLiteral("接收状态"),
        QStringLiteral("平均接收速率"),
        QStringLiteral("接收时间")
    });

    m_table->setStyleSheet(
        R"(
        QTableWidget {
            gridline-color: #d8e0ec;
            background-color: #f8fafc;
            color: #333333;
            font-size: 13px;
            border: none;
            selection-background-color: #e0ebfa;
        }
        QTableWidget::item { padding: 6px 10px; }
        QHeaderView::section {
            background-color: #e8eef6;
            color: #555555;
            font-size: 13px;
            font-weight: bold;
            padding: 8px 12px;
            border: none;
            border-bottom: 1px solid #c8d4e8;
        }
        )"
    );

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(true);
    m_table->setAlternatingRowColors(true);

    QHeaderView *header = m_table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->resizeSection(0, 84);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->resizeSection(1, 124);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Fixed);
    header->resizeSection(3, 100);
    header->setSectionResizeMode(4, QHeaderView::Fixed);
    header->resizeSection(4, 126);
    header->setSectionResizeMode(5, QHeaderView::Fixed);
    header->resizeSection(5, 164);

    m_table->setMinimumHeight(180);
    layout->addWidget(m_table);
}

int ReceiveHistoryTab::findRowByTaskId(unsigned int taskId) const
{
    for (int row = 0; row < m_table->rowCount(); ++row) {
        auto *item = m_table->item(row, 0);
        if (item && item->text().toUInt() == taskId) {
            return row;
        }
    }
    return -1;
}

QString ReceiveHistoryTab::formatRate(float kbps)
{
    if (kbps >= 1024.0f) {
        return QString("%1 Mbps").arg(kbps / 1024.0f, 0, 'f', 2);
    }
    return QString("%1 kbps").arg(kbps, 0, 'f', 1);
}

void ReceiveHistoryTab::applyStatusColor(QTableWidgetItem *statusItem, const QString &status)
{
    if (!statusItem) {
        return;
    }
    if (status.contains(QStringLiteral("完成")) || status == QStringLiteral("接收成功")) {
        statusItem->setForeground(QColor("#28a060"));
    } else if (status.contains(QStringLiteral("中断")) || status == QStringLiteral("接收失败")) {
        statusItem->setForeground(QColor("#dc3545"));
    } else {
        statusItem->setForeground(QColor("#2060b0"));
    }
}

void ReceiveHistoryTab::addRecvRecord(unsigned int taskId,
                                      unsigned int senderTid,
                                      const QString &fileName,
                                      const QString &status,
                                      float avgSpeedKbps,
                                      const QString &recvTime)
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);

    m_table->setItem(row, 0, new QTableWidgetItem(QString::number(taskId)));
    m_table->setItem(row, 1, new QTableWidgetItem(senderTid == 0 ? QStringLiteral("--") : QString::number(senderTid)));
    m_table->setItem(row, 2, new QTableWidgetItem(fileName));

    auto *statusItem = new QTableWidgetItem(status);
    applyStatusColor(statusItem, status);
    m_table->setItem(row, 3, statusItem);

    m_table->setItem(row, 4, new QTableWidgetItem(formatRate(avgSpeedKbps)));
    m_table->setItem(row, 5, new QTableWidgetItem(recvTime));
}

void ReceiveHistoryTab::upsertRecvProgress(unsigned int taskId,
                                           unsigned int senderTid,
                                           const QString &fileName,
                                           const QString &status,
                                           float rateKbps,
                                           const QString &recvTime)
{
    const int row = findRowByTaskId(taskId);
    if (row < 0) {
        addRecvRecord(taskId, senderTid, fileName, status, rateKbps, recvTime);
        return;
    }

    if (auto *tidItem = m_table->item(row, 1)) {
        if (senderTid != 0) {
            tidItem->setText(QString::number(senderTid));
        }
    }
    if (auto *nameItem = m_table->item(row, 2)) {
        nameItem->setText(fileName);
    }
    if (auto *statusItem = m_table->item(row, 3)) {
        statusItem->setText(status);
        applyStatusColor(statusItem, status);
    }
    if (auto *rateItem = m_table->item(row, 4)) {
        if (rateKbps > 0.0f) {
            rateItem->setText(formatRate(rateKbps));
        }
    }
    if (auto *timeItem = m_table->item(row, 5)) {
        timeItem->setText(recvTime);
    }
}

void ReceiveHistoryTab::updateRecvStatus(unsigned int taskId, const QString &newStatus)
{
    const int row = findRowByTaskId(taskId);
    if (row < 0) {
        return;
    }
    auto *statusItem = m_table->item(row, 3);
    if (!statusItem) {
        return;
    }

    statusItem->setText(newStatus);
    applyStatusColor(statusItem, newStatus);
}

void ReceiveHistoryTab::clearRecords()
{
    m_table->setRowCount(0);
}
