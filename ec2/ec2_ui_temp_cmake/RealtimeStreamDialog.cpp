#include "RealtimeStreamDialog.h"
#include "VideoSender.h"
#include "VideoReceiver.h"
#include "playerwidget.h"
#include "AutoConnController.h"
#include "function.h"

// Defined in main_autoconn.cpp
extern VideoReceiver *g_videoReceiver;

RealtimeStreamDialog::RealtimeStreamDialog(AutoConnController *ctrl, QWidget *parent)
    : QDialog(parent)
    , m_ctrl(ctrl)
{
    setWindowTitle(QStringLiteral("实时音视频传输"));
    setMinimumSize(760, 680);
    resize(840, 720);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // ---- Top row: target selector + button ----
    auto *topRow = new QHBoxLayout();
    auto *targetLabel = new QLabel(QStringLiteral("目标终端："), this);
    targetLabel->setStyleSheet("font-size: 14px; color: #333; font-weight: bold;");
    m_targetCombo = new QComboBox(this);
    m_targetCombo->setMinimumWidth(120);
    m_targetCombo->setStyleSheet(
        "QComboBox { border: 1px solid #c8d4e8; border-radius: 4px; "
        "padding: 4px 8px; font-size: 14px; background: #f8fafc; }");

    m_startStopBtn = new QPushButton(QStringLiteral("开始发送"), this);
    m_startStopBtn->setMinimumSize(120, 36);
    m_startStopBtn->setEnabled(false);
    m_startStopBtn->setStyleSheet(
        "QPushButton { background-color: #28a060; color: white; border-radius: 6px; "
        "font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #38b070; }"
        "QPushButton:pressed { background-color: #188050; }"
        "QPushButton:disabled { background-color: #aaaaaa; color: #dddddd; }");

    topRow->addWidget(targetLabel);
    topRow->addWidget(m_targetCombo, 1);
    topRow->addSpacing(12);
    topRow->addWidget(m_startStopBtn);
    mainLayout->addLayout(topRow);

    // ---- Split preview area ----
    m_previewContainer = new QWidget(this);
    m_previewContainer->setStyleSheet("QWidget { background-color: #1a1a2e; border-radius: 8px; }");
    auto *previewLayout = new QVBoxLayout(m_previewContainer);
    previewLayout->setContentsMargins(6, 6, 6, 6);
    previewLayout->setSpacing(4);

    // TX area (sender preview) - top half
    m_txArea = new QWidget(m_previewContainer);
    m_txArea->setMinimumHeight(200);
    auto *txLayout = new QVBoxLayout(m_txArea);
    txLayout->setContentsMargins(0, 0, 0, 0);
    auto *txLabel = new QLabel(QStringLiteral("本端画面 (发送预览)"), m_txArea);
    txLabel->setStyleSheet("QLabel { color: #aaa; font-size: 12px; background: transparent; }");
    txLabel->setAlignment(Qt::AlignCenter);
    m_txStatusLabel = txLabel;
    txLayout->addWidget(txLabel);

    // RX area (receiver video) - bottom half
    m_rxArea = new QWidget(m_previewContainer);
    m_rxArea->setMinimumHeight(200);
    auto *rxLayout = new QVBoxLayout(m_rxArea);
    rxLayout->setContentsMargins(0, 0, 0, 0);

    // Embed receiver's playerWidget if available (receiver auto-starts at app launch)
    if (g_videoReceiver && g_videoReceiver->playerWidget) {
        rxLayout->addWidget(g_videoReceiver->playerWidget);
    } else {
        auto *rxLabel = new QLabel(QStringLiteral("对端画面 (接收中...)"), m_rxArea);
        rxLabel->setStyleSheet("QLabel { color: #aaa; font-size: 12px; background: transparent; }");
        rxLabel->setAlignment(Qt::AlignCenter);
        rxLayout->addWidget(rxLabel);
    }

    previewLayout->addWidget(m_txArea, 1);
    previewLayout->addWidget(m_rxArea, 1);
    mainLayout->addWidget(m_previewContainer, 1);

    // ---- Bottom info ----
    auto *bottomRow = new QHBoxLayout();
    auto *infoLabel = new QLabel(
        QStringLiteral("源文件: FileSend/videotest.mp4 | 双向实时传输, 自动码率适配"), this);
    infoLabel->setStyleSheet("font-size: 11px; color: #777;");
    bottomRow->addWidget(infoLabel);
    bottomRow->addStretch();
    mainLayout->addLayout(bottomRow);

    // ---- Connections ----
    connect(m_startStopBtn, &QPushButton::clicked,
            this, &RealtimeStreamDialog::onStartStopClicked);
    connect(m_targetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RealtimeStreamDialog::onTargetChanged);
    if (m_ctrl) {
        connect(m_ctrl, &AutoConnController::terminalListChanged,
                this, &RealtimeStreamDialog::onTerminalListChanged, Qt::QueuedConnection);
    }
}

RealtimeStreamDialog::~RealtimeStreamDialog()
{
    stopSending();
    // Return receiver widget to its original parent so VideoReceiver destructor can clean up
    if (g_videoReceiver && g_videoReceiver->playerWidget) {
        auto *rxLayout = m_rxArea->layout();
        if (rxLayout) {
            rxLayout->removeWidget(g_videoReceiver->playerWidget);
        }
        g_videoReceiver->playerWidget->setParent(g_videoReceiver);
    }
}

void RealtimeStreamDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    // Clear stale frame from receiver when dialog is shown
    if (g_videoReceiver && g_videoReceiver->playerWidget) {
        g_videoReceiver->playerWidget->clearDisplay();
    }
}

void RealtimeStreamDialog::setTerminalList(const QStringList &tids)
{
    onTerminalListChanged(tids);
}

void RealtimeStreamDialog::onTerminalListChanged(const QStringList &tids)
{
    const QString current = m_targetCombo->currentText();
    m_targetCombo->clear();
    m_targetCombo->addItems(tids);
    if (!current.isEmpty()) {
        const int idx = m_targetCombo->findText(current);
        if (idx >= 0) {
            m_targetCombo->setCurrentIndex(idx);
        }
    }
    m_startStopBtn->setEnabled(m_targetCombo->count() > 0 && !m_sending);
}

void RealtimeStreamDialog::onTargetChanged(int)
{
    if (!m_sending) {
        m_startStopBtn->setEnabled(m_targetCombo->count() > 0);
    }
}

void RealtimeStreamDialog::onStartStopClicked()
{
    if (m_sending) {
        stopSending();
        return;
    }

    bool ok = false;
    const int targetTID = m_targetCombo->currentText().toInt(&ok);
    if (!ok || targetTID <= 0) {
        QMessageBox::warning(this, QStringLiteral("错误"),
                             QStringLiteral("请选择有效的目标终端。"));
        return;
    }

    // Create VideoSender (hidden, we only use its playerWidget + pipeline)
    m_sender = new VideoSender(this);
    m_sender->hide();

    // Embed sender's playerWidget into TX area
    auto *txLayout = qobject_cast<QVBoxLayout *>(m_txArea->layout());
    if (txLayout) {
        m_txStatusLabel->hide();
        txLayout->addWidget(m_sender->playerWidget);
    }

    // Start sending to the selected TID
    m_sender->startDeepIntegrationPipeline(targetTID);

    m_sending = true;
    m_startStopBtn->setText(QStringLiteral("停止发送"));
    m_startStopBtn->setStyleSheet(
        "QPushButton { background-color: #c0392b; color: white; border-radius: 6px; "
        "font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #e74c3c; }"
        "QPushButton:pressed { background-color: #a93226; }");
    m_targetCombo->setEnabled(false);
}

void RealtimeStreamDialog::stopSending()
{
    if (m_sender) {
        auto *txLayout = qobject_cast<QVBoxLayout *>(m_txArea->layout());
        if (txLayout && m_sender->playerWidget) {
            txLayout->removeWidget(m_sender->playerWidget);
        }
        delete m_sender;
        m_sender = nullptr;
    }

    m_sending = false;
    m_startStopBtn->setText(QStringLiteral("开始发送"));
    m_startStopBtn->setStyleSheet(
        "QPushButton { background-color: #28a060; color: white; border-radius: 6px; "
        "font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #38b070; }"
        "QPushButton:pressed { background-color: #188050; }"
        "QPushButton:disabled { background-color: #aaaaaa; color: #dddddd; }");
    m_targetCombo->setEnabled(true);
    m_txStatusLabel->show();
    m_txStatusLabel->setText(QStringLiteral("本端画面 (发送预览)"));
    m_startStopBtn->setEnabled(m_targetCombo->count() > 0);
}
