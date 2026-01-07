//
// Created by 王炳棋 on 2022/11/3.
//

#ifndef EPOLLCOMM_CHANNELMANAGER_H
#define EPOLLCOMM_CHANNELMANAGER_H

//channelmanager中没有加锁，也没有对数组索引进行安全判断，在外部进行设计。

#include "CIPSOCKET.h"

#define UnKnowChannel 0//未使用的通道
#define ClientChannel 1 //ClientChannel
#define ServerChannel 2 //未在使用的ServerChannel
#define ServerTransChannel 3//正在使用的ServerChannel
#define ServerListenChannel 4//总的ServerChannel

class ChannelManager {
public:
    ChannelManager() {
        pthread_mutex_init(&Chmtx, nullptr);
        bzero(ChannelStat, sizeof(ChannelStat));
        //printf("ChannelManager Ready\n");
    }

    ~ChannelManager() {
        //printf("ChannelManager Close\n");
        pthread_mutex_destroy(&Chmtx);
    }

public:
    inline void lock() {
        pthread_mutex_lock(&Chmtx);
    }

    void unlock() {
        pthread_mutex_unlock(&Chmtx);
    }

    short GetChannelStat(DWORD dwChannel);

    bool IsClientAvailable(DWORD dwChannel);

    bool IsServerAvailable(DWORD dwChannel);

    bool IsTransChannelAvailable(DWORD dwChannel);

    bool IsListenChannelAvailable(DWORD dwChannel);

    void SetChannelStat(DWORD dwChannel, short stat);

    void SetChannelStat(DWORD ChannelHead, DWORD ChannelEnd, short stat);

private:
    pthread_mutex_t Chmtx;
    short ChannelStat[MAXCHANNELNUM];//0~9999
};

#endif //EPOLLCOMM_CHANNELMANAGER_H
