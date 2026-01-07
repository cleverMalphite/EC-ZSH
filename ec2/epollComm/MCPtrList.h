//
// Created by 王炳棋 on 2022/10/26.
//

#ifndef EPOLLCOMM_MCPTRLIST_H
#define EPOLLCOMM_MCPTRLIST_H

#include <iostream>
#include <vector>
#include <memory>

using std::vector;
using std::shared_ptr;

typedef struct node {
    uint32_t ChannelNumber;
} NODE;

//线程不安全

template<typename T>
class MCPtrList {
public:
    MCPtrList() {
        pthread_mutex_init(&McpMtx, nullptr);
    }

    ~MCPtrList() {
        pthread_mutex_destroy(&McpMtx);
    }

    using size_cptr = size_t;

    inline void Lock() {
        pthread_mutex_lock(&McpMtx);
    }

    inline void UnLock() {
        pthread_mutex_unlock(&McpMtx);
    }

    shared_ptr<T> GetAt(size_cptr pos);//返回下标pos的元素
    shared_ptr<T> GetNext(size_cptr pos);//返回下标pos的后一个元素
    bool RemoveAt(size_cptr pos);//删除pos位置的元素，如果删除则返回true
    size_cptr GetPosition();//返回一个size_cptr变量，用于for循环中的初始化等
    size_cptr GetHeadPosition();//得到第一个元素的下标，一般是1
    shared_ptr<T> GetTail();//获取尾元素
    bool AddTail(shared_ptr<T> node);//在尾后插入一个元素
    bool IsEmpty();//无元素时返回true，否则返回false
    shared_ptr<T> RemoveHead();//删除首元素
    size_cptr Size() { return nodeList.size(); };//返回元素个数
public:
    vector<shared_ptr<NODE>> nodeList;
private:
    pthread_mutex_t McpMtx;

};

/*template<typename T>
MCPtrList<T>::MCPtrList() = default;

template<typename T>
MCPtrList<T>::~MCPtrList() = default;*/



template<typename T>
shared_ptr<T> MCPtrList<T>::GetAt(size_cptr pos) {
    if (pos < nodeList.size()) {
        return nodeList[pos];
    } else {
        return nullptr;
    }
}

template<typename T>
shared_ptr<T> MCPtrList<T>::GetNext(size_cptr pos) {
    if (pos < nodeList.size()) {
        return nodeList[pos + 1];
    } else {
        return nullptr;
    }

}

/**
 * 这个函数用来删除第N个元素，但是要注意的是pos是从0开始
 * 如果删除成功会返回true，否则会返回FALSE
 */
template<typename T>
bool MCPtrList<T>::RemoveAt(size_cptr pos) {
    auto tempNode = nodeList.begin();
    for (auto i = nodeList.begin(); i != nodeList.end() && pos >= 0; i++, pos--) {
        tempNode = i;
    }
    //如果pos不为0，那么就代表没有该下标对应的元素
    if (pos >= 0) {
        return false;
    }
    //20210329修改日志:修正了无法正确删除指定位置元素的bug,for循环pos为0后还会在循环一次变成-1
    if (-1 == pos) {
        nodeList.erase(tempNode);
        return true;
    }
    return true;
}

template<typename T>
size_t MCPtrList<T>::GetPosition() {
    size_cptr i;
    return i;
}

template<typename T>
size_t MCPtrList<T>::GetHeadPosition() {
    size_cptr head = 0;
    return head;
}

template<typename T>
shared_ptr<T> MCPtrList<T>::GetTail() {
    if (nodeList.size() > 0) {
        return nodeList.back();
    }
    shared_ptr<T> result = nullptr;
    return result;
}

template<typename T>
bool MCPtrList<T>::AddTail(shared_ptr<T> node) {
    nodeList.push_back(node);
    return true;
}

template<typename T>
bool MCPtrList<T>::IsEmpty() {
    return nodeList.size() > 0 ? false : true;
}

template<typename T>
shared_ptr<T> MCPtrList<T>::RemoveHead() {
    shared_ptr<T> headNode = nodeList.front();
    nodeList.erase(nodeList.begin());
    return headNode;
}

#endif //EPOLLCOMM_MCPTRLIST_H
