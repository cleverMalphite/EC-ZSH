//
// Created by ubuntu402 on 23-11-5.
//

#ifndef EMERGENCYCOMMUNICATION_DATAUPDATER_H
#define EMERGENCYCOMMUNICATION_DATAUPDATER_H
#include <QObject>

class DataUpdater :public QObject{
    Q_OBJECT

public:
    DataUpdater(){};

signals:
    void updateTerminalSignal();
    void updateSendProgressSignal(unsigned int task_id);
    void updateSendStatusSignal();
    void updateRecvProgressSignal(unsigned int task_id);
    void updateRecvStatusSignal();
    void updateSendTaskSignal();
    void updateRecvTaskSignal();
    void startSendVideo_signal(unsigned int remote_tid,unsigned int remote_videoPort);
    void endSendVideo_signal();
    void recvVideoData_signal(QByteArray data,unsigned int dataLength,unsigned int packetIndex);
    void remoteIsBusy_signal(unsigned int remote_tid);
};


#endif //EMERGENCYCOMMUNICATION_DATAUPDATER_H
