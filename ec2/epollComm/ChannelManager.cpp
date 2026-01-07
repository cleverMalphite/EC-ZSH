//
// Created by 王炳棋 on 2022/11/4.
//

#include "ChannelManager.h"

short ChannelManager::GetChannelStat(DWORD dwChannel) {
    //pthread_mutex_lock(&Chmtx);
    short TempStat = ChannelStat[dwChannel];
    //pthread_mutex_unlock(&Chmtx);
    return TempStat;
}

bool ChannelManager::IsClientAvailable(DWORD dwChannel) {
    //pthread_mutex_lock(&Chmtx);
    if (UnKnowChannel == ChannelStat[dwChannel]) {
        //pthread_mutex_unlock(&Chmtx);
        return true;
    }
    //pthread_mutex_unlock(&Chmtx);
    return false;
}

bool ChannelManager::IsServerAvailable(DWORD dwChannel) {
    //pthread_mutex_lock(&Chmtx);
    if (UnKnowChannel == ChannelStat[dwChannel]) {
        //pthread_mutex_unlock(&Chmtx);
        return true;
    }
    //pthread_mutex_unlock(&Chmtx);
    return false;
}

bool ChannelManager::IsTransChannelAvailable(DWORD dwChannel) {
    //pthread_mutex_lock(&Chmtx);
    if (ServerChannel == ChannelStat[dwChannel]) {
        //pthread_mutex_unlock(&Chmtx);
        return true;
    }
    //pthread_mutex_unlock(&Chmtx);
    return false;
}

bool ChannelManager::IsListenChannelAvailable(DWORD dwChannel) {
    //pthread_mutex_lock(&Chmtx);
    if (ServerChannel == ChannelStat[dwChannel]) {
        //pthread_mutex_unlock(&Chmtx);
        return true;
    }
    //pthread_mutex_unlock(&Chmtx);
    return false;
}

void ChannelManager::SetChannelStat(DWORD dwChannel, short stat) {
    //pthread_mutex_lock(&Chmtx);
    ChannelStat[dwChannel] = stat;
    //pthread_mutex_unlock(&Chmtx);
}

void ChannelManager::SetChannelStat(DWORD ChannelHead, DWORD ChannelEnd, short stat) {
    //pthread_mutex_lock(&Chmtx);
    for (DWORD dwChannel = ChannelHead; dwChannel != ChannelEnd; dwChannel++) {
        ChannelStat[dwChannel] = stat;
    }
    //pthread_mutex_unlock(&Chmtx);
}
