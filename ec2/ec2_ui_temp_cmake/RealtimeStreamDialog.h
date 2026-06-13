#ifndef REALTIMESTREAMDIALOG_H
#define REALTIMESTREAMDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QShowEvent>

class VideoSender;
class VideoReceiver;
class PlayerWidget;
class AutoConnController;

class RealtimeStreamDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RealtimeStreamDialog(AutoConnController *ctrl, QWidget *parent = nullptr);
    ~RealtimeStreamDialog();

    void setTerminalList(const QStringList &tids);

private slots:
    void onStartStopClicked();
    void onTargetChanged(int index);
    void onTerminalListChanged(const QStringList &tids);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void stopSending();

    AutoConnController *m_ctrl = nullptr;
    VideoSender        *m_sender = nullptr;
    QWidget            *m_previewContainer = nullptr;
    QWidget            *m_txArea = nullptr;
    QWidget            *m_rxArea = nullptr;
    QComboBox          *m_targetCombo = nullptr;
    QPushButton        *m_startStopBtn = nullptr;
    QLabel             *m_txStatusLabel = nullptr;
    bool                m_sending = false;
};

#endif // REALTIMESTREAMDIALOG_H
