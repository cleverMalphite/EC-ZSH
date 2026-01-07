//
// Created by 王炳棋 on 2022/11/14.
//

#ifndef NETCOMBTRANSFER_CNETCOMBTRANSFERBUFFERLIST_H
#define NETCOMBTRANSFER_CNETCOMBTRANSFERBUFFERLIST_H
#define _BUFFER_LIST_LOCK_

#include "../mGlobalDef.h"
#include <vector>
#include <deque>

using std::shared_ptr;

template<typename T>
class NETCombTransferBufferList {
public:
    NETCombTransferBufferList();

    ~NETCombTransferBufferList();

    bool m_bInUse;    //标识现在的队列是否处于忙的状态
    pthread_mutex_t NCTBLmtx{};

    void Lock() {
#ifdef _BUFFER_LIST_LOCK_
        pthread_mutex_lock(&NCTBLmtx);
#endif
    }

    void UnLock() {
#ifdef _BUFFER_LIST_LOCK_
        pthread_mutex_unlock(&NCTBLmtx);
#endif
    }

public:
    int GetMyCount();    //返回当前deque的元素数目
    std::shared_ptr<T> GetMyHead(); //返回当前deque第一个元素的引用
    shared_ptr<T> GetMyTail(); //返回当前deque最后一个元素的引用

    void AddTail(shared_ptr<T> pData);    //向当前deque添加尾元素
    shared_ptr<T> RemoveHead();    //删除deque的首元素，并返回其值
    void RemoveAt(shared_ptr<T> pData);
    std::shared_ptr<T> GetNextNode(const std::shared_ptr<T>& currentNode);   //返回下一个节点元素

public:
    std::deque<shared_ptr<T>> node_list;

};

template<typename T>
NETCombTransferBufferList<T>::NETCombTransferBufferList() {
    //清空vector中的所有元素
    node_list.clear();
    m_bInUse = false;    //标记元素还未使用
    pthread_mutex_init(&NCTBLmtx, nullptr);    //初始化资源锁
}

template<typename T>
NETCombTransferBufferList<T>::~NETCombTransferBufferList() {
    pthread_mutex_destroy(&NCTBLmtx);   //删除资源锁
}

template<typename T>
int NETCombTransferBufferList<T>::GetMyCount() {
    Lock();
    int number = 0;
    try {
        number = node_list.size();
    }
    catch (...) {
        UnLock();
        return 0;
    }
    UnLock();
    return number;
}

template<typename T>
std::shared_ptr<T> NETCombTransferBufferList<T>::GetMyHead() {
    Lock();
    shared_ptr<T> node = nullptr;
    try {
        node = node_list.front();
    }
    catch (...) {
        UnLock();
        return nullptr;
    }
    UnLock();
    return node;

}

template<typename T>
shared_ptr<T> NETCombTransferBufferList<T>::GetMyTail() {
    Lock();
    shared_ptr<T> node = nullptr;
    try {
        node = node_list.back();
    }
    catch (...) {
        UnLock();
        return nullptr;
    }
    UnLock();
    return node;
}

template<typename T>
void NETCombTransferBufferList<T>::AddTail(shared_ptr<T> pData) {
    Lock();
    try {
        node_list.push_back(pData);
    }
    catch (...) {
        UnLock();
        return;
    }
    UnLock();
}

template<typename T>
shared_ptr<T> NETCombTransferBufferList<T>::RemoveHead() {
    Lock();
    try {
        if (node_list.empty()) {
            UnLock();
            return nullptr;
        }
        auto node = node_list.front();
        node_list.pop_front();
        UnLock();
        return node;
    }
    catch (...) {
        UnLock();
        return nullptr;
    }
    UnLock();
}

template<typename T>
void NETCombTransferBufferList<T>::RemoveAt(shared_ptr<T> pData) {
    Lock();
    try {
        if (!pData || !pData.get()) {
            UnLock();
            return;
        }
        auto node = node_list.begin();
        while (node != node_list.end()) {
            if ((*node) && (*node) == pData) {
                node_list.erase(node);
                //printf("[NCT::AutoTestDebug]::删除了对应通道\n");
                UnLock();
                return;
            }
            ++node;
        }
    }
    catch (...) {
        UnLock();
        return;
    }
    UnLock();
}

template<typename T>
std::shared_ptr<T> NETCombTransferBufferList<T>::GetNextNode(const std::shared_ptr<T>& currentNode) {
    std::shared_ptr<T> nextNode = nullptr;
    try {
        auto it = std::find(node_list.begin(), node_list.end(), currentNode);
        if (it != node_list.end()) {
            if (std::next(it) != node_list.end()) {
                nextNode = *std::next(it);
            } else {
                // Current node is the last one, return the first node
                nextNode = node_list.front();
            }
        }
    }
    catch (...) {
        return nullptr;
    }
    return nextNode;
}



#endif //NETCOMBTRANSFER_CNETCOMBTRANSFERBUFFERLIST_H
