//
// Created by 王炳棋 on 2022/11/16.
//

#ifndef NETCOMBTRANSFER_CNETTERMINAL_H
#define NETCOMBTRANSFER_CNETTERMINAL_H

#include "CNetChannel.h"
#include "CNetCombTransferBufferList.h"

class CNetTerminal {
public:
    CNetTerminal();

    virtual ~CNetTerminal();

public:
    NETCombTransferBufferList<CNetChannel> NetChannelList;
    unsigned long m_dwTID;  //终端标志
    unsigned m_nQoSSend;
    unsigned m_nQoSRecv;
    shared_ptr<CNetChannel> Current_NextChannel= nullptr;
    shared_ptr<CNetChannel> Current_NextChannel1= nullptr;
    shared_ptr<CNetChannel> Current_NextChannel2=nullptr;
    shared_ptr<CNetChannel> Current_NextChannel3=nullptr;

public:

    /*bool SetConnect();*/

    std::shared_ptr<CNetChannel> ChannelSelect();   //选择最优通道

    bool CreateNetChannel(DWORD nCID);   //创建通道类

    int AddNetChannel(std::shared_ptr<CNetChannel> pNetChannel);    //增加通道类

    DWORD RemoveNetChannel(const std::shared_ptr<CNetChannel> &pNetChannel);   //删除通道类

    bool ChannelTraverse(); //遍历临时终端里所有的通道号，并进行状态轮转

    std::shared_ptr<CNetChannel> ChannelTraverse(DWORD nCID);   //查找通道号nCID对应的通道

    //介于底层用了最新的通道处理机制没有，要求通道号重拍，后续会进一步优化
    /* void ReloadNetChannel(DWORD nCID);  //重拍通道链表，将剩余通道值减1（针对的是原有的底层的通道管理机制）*/
};

inline bool operator==(const CNetTerminal &lhs, const CNetTerminal &rhs) {
    return lhs.m_dwTID == rhs.m_dwTID;
}

inline bool operator!=(const CNetTerminal &lhs, const CNetTerminal &rhs) {
    return !(operator==(lhs, rhs));
}

#endif //NETCOMBTRANSFER_CNETTERMINAL_H
