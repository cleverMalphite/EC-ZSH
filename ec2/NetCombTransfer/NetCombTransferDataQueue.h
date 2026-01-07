//
// Created by 王炳棋 on 2022/11/14.
//

#ifndef NETCOMBTRANSFER_NETCOMBTRANSFERDATAQUEUE_H
#define NETCOMBTRANSFER_NETCOMBTRANSFERDATAQUEUE_H

#include <queue>
#include "../mGlobalDef.h"

using std::shared_ptr;
using std::queue;

/*提供线程安全的队列*/
template<typename T>
class NetCombTransferDataQueue {
public:
    NetCombTransferDataQueue();

    ~NetCombTransferDataQueue();

    bool m_bInUse;
    pthread_mutex_t NCTDQmtx;

    void Lock() {
        pthread_mutex_lock(&NCTDQmtx);
    }

    void UnLock() {
        pthread_mutex_unlock(&NCTDQmtx);
    }

public:
    UINT GetMyCount();

    std::shared_ptr<T> GetMyHead();

    std::shared_ptr<T> GetMyTail();

    void AddTail(std::shared_ptr<T> pData);

    std::shared_ptr<T> RemoveHead();

    void RemoveAt(std::shared_ptr<T> pData);

public:
    std::queue<std::shared_ptr<T>> node_list;
};

template<typename T>
NetCombTransferDataQueue<T>::NetCombTransferDataQueue() {
    m_bInUse = false;
    pthread_mutex_init(&NCTDQmtx, nullptr);
}

template<typename T>
NetCombTransferDataQueue<T>::~NetCombTransferDataQueue() {
    pthread_mutex_destroy(&NCTDQmtx);
}

template<typename T>
UINT NetCombTransferDataQueue<T>::GetMyCount() {
    Lock();
    UINT Counts = 0;
    try {
        Counts = node_list.size();
    } catch (...) {
        UnLock();
        return 0;
    }
    UnLock();
    return Counts;
}

template<typename T>
std::shared_ptr<T> NetCombTransferDataQueue<T>::GetMyHead() {
    Lock();
    std::shared_ptr<T> node = nullptr;
    try {
        node = node_list.front();
    } catch (...) {
        UnLock();
        return nullptr;
    }
    UnLock();
    return node;
}

template<typename T>
std::shared_ptr<T> NetCombTransferDataQueue<T>::GetMyTail() {
    Lock();
    std::shared_ptr<T> node = nullptr;
    try {
        node = node_list.back();
    } catch (...) {
        UnLock();
        return nullptr;
    }
    UnLock();
    return node;
}

template<typename T>
void NetCombTransferDataQueue<T>::AddTail(std::shared_ptr<T> pData) {
    Lock();
    try {
        node_list.push(pData);
    } catch (...) {
        UnLock();
        return;
    }
    UnLock();
}

template<typename T>
std::shared_ptr<T> NetCombTransferDataQueue<T>::RemoveHead() {
    Lock();
    try {
        if (0 == node_list.size()) {
            UnLock();
            return nullptr;
        }
        auto node = node_list.front();
        node_list.pop();
        UnLock();
        return node;
    } catch (...) {
        UnLock();
        return nullptr;
    }
}

#endif //NETCOMBTRANSFER_NETCOMBTRANSFERDATAQUEUE_H
