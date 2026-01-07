//
// Created by wangbingqi on 22-9-30.
//

#ifndef EPOLLCOMM_CNETCHANNELLIST_H
#define EPOLLCOMM_CNETCHANNELLIST_H

//todo:对通道的操作是否要被设计在此处？目前仅是对通道队列的增减进行了操作，没有涉及到对通道号的管理，对通道号的管理需要调用其他函数:reloadchannel,resortchannel等...

#include <vector>
#include <memory>
#include <pthread.h>

template<typename T>
class CNetChannelist {
public:
    CNetChannelist() {
        pthread_mutex_init(&m_channelistmtx, NULL);
    }

    ~CNetChannelist() {
        pthread_mutex_destroy(&m_channelistmtx);
    }

public:
    int GetNetnum();

    std::shared_ptr<T> GetMyHead();

    std::shared_ptr<T> GetMyTail();

    pthread_mutex_t m_channelistmtx;

    bool AddTail(std::shared_ptr<T> pChannel);

    std::shared_ptr<T> RemoveHead();

    bool RemoveAt(std::shared_ptr<T> TempTcp);

    std::shared_ptr<T> RemoveAt(std::string remove_ipAddress);

    std::shared_ptr<T> GetByChannelNum(DWORD ChannelNum);

    std::shared_ptr<T> GetBySocket(int SockNum);

public:
    inline void Lock() {
        pthread_mutex_lock(&m_channelistmtx);
    }

    inline void UnLock() {
        pthread_mutex_unlock(&m_channelistmtx);
    }

public:
    std::vector<std::shared_ptr<T>> NetChannel;
    //多线程：注意vector的迭代器失效
};

template<typename T>
int CNetChannelist<T>::GetNetnum() {
    pthread_mutex_lock(&m_channelistmtx);
    int ChannelCounts = NetChannel.size();
    pthread_mutex_unlock(&m_channelistmtx);
    return ChannelCounts;
}

template<typename T>
std::shared_ptr<T> CNetChannelist<T>::GetMyHead() {
    pthread_mutex_lock(&m_channelistmtx);
    std::shared_ptr<T> TempChannel = NetChannel.front();
    pthread_mutex_unlock(&m_channelistmtx);
    return TempChannel;
}

template<typename T>
std::shared_ptr<T> CNetChannelist<T>::GetMyTail() {
    pthread_mutex_lock(&m_channelistmtx);
    std::shared_ptr<T> TempChannel = NetChannel.back();
    pthread_mutex_unlock(&m_channelistmtx);
    return TempChannel;
}

template<typename T>
bool CNetChannelist<T>::AddTail(std::shared_ptr<T> pChannel) {
    if (!pChannel || !pChannel.get()) return false;
    pthread_mutex_lock(&m_channelistmtx);
    NetChannel.push_back(pChannel);
    pthread_mutex_unlock(&m_channelistmtx);
    return true;
}

template<typename T>
std::shared_ptr<T> CNetChannelist<T>::RemoveHead() {
    pthread_mutex_lock(&m_channelistmtx);
    std::shared_ptr<T> TempChannel = NetChannel.front();
    NetChannel.erase(NetChannel.begin());
    pthread_mutex_unlock(&m_channelistmtx);
    return TempChannel;
}

//或者说这个函数更应该称为removebychannelnum
template<typename T>
bool CNetChannelist<T>::RemoveAt(std::shared_ptr<T> TempTcp) {
    if (!TempTcp || !TempTcp.get()) return false;
    pthread_mutex_lock(&m_channelistmtx);
    /*auto TempChannel = NetChannel.begin();
    while (TempChannel != NetChannel.end()) {
        if ((*TempChannel) && TempTcp && (*TempChannel)->GetChannel() == TempTcp->GetChannel()) {
            NetChannel.erase(TempChannel);
            pthread_mutex_unlock(&m_channelistmtx);
            return true;
        }
        ++TempChannel;
    }*/
    for (auto index = NetChannel.begin(); index != NetChannel.end(); index++) {
        if ((*index) && (*index)->GetChannel() == TempTcp->GetChannel()) {
            NetChannel.erase(index);
            pthread_mutex_unlock(&m_channelistmtx);
            return true;
        }
    }
    pthread_mutex_unlock(&m_channelistmtx);
    return false;
}

template<typename T>
std::shared_ptr<T> CNetChannelist<T>::RemoveAt(std::string remove_ipAddress) {
    pthread_mutex_lock(&m_channelistmtx);
    auto TempChannel = NetChannel.begin();
    while (TempChannel != NetChannel.end()) {
        //todo: finish GetLocalHostAddress()
        if ((TempChannel->GetLocalHostAddress()) == remove_ipAddress) {
            TempChannel = NetChannel.erase(socket);
            pthread_mutex_unlock(&m_channelistmtx);
            return TempChannel;
        }
    }
    pthread_mutex_unlock(&m_channelistmtx);
    return nullptr;
}

//need test : 遍历是否出错
template<typename T>
std::shared_ptr<T> CNetChannelist<T>::GetBySocket(int SockNum) {
    pthread_mutex_lock(&m_channelistmtx);
    for (auto channel: NetChannel) {
        if (channel && channel->mLocalSock == SockNum) {
            pthread_mutex_unlock(&m_channelistmtx);
            return channel;
        }
    }
    pthread_mutex_unlock(&m_channelistmtx);
    return nullptr;
}

template<typename T>
std::shared_ptr<T> CNetChannelist<T>::GetByChannelNum(DWORD ChannelNum) {
    pthread_mutex_lock(&m_channelistmtx);
    for (auto channel: NetChannel) {
        if (channel && channel->m_dwChannel == ChannelNum) {
            pthread_mutex_unlock(&m_channelistmtx);
            return channel;
        }
    }
    pthread_mutex_unlock(&m_channelistmtx);
    return nullptr;
}

#endif //EPOLLCOMM_CNETCHANNELLIST_H
