//
// Created by 王炳棋 on 2022/11/16.
//

#include "CNetTerminal.h"

#include <utility>

//根据linux多线程编程描述，最好使用lock guard，不过咱们的项目中关于锁的用法并不统一，后续可以优化

CNetTerminal::CNetTerminal() :
        m_dwTID(0),
        m_nQoSRecv(0), m_nQoSSend(0) {

}

CNetTerminal::~CNetTerminal() {
    NetChannelList.node_list.clear();
}
bool flag=true,flag1=true;
int count=0;
std::shared_ptr<CNetChannel> CNetTerminal::ChannelSelect() {
    if(Current_NextChannel == nullptr){
        Current_NextChannel=NetChannelList.GetMyHead();
    }
/*
    if(flag){
        if(NetChannelList.GetMyCount()==3){
            printf("channel size: 3 \n");
            flag=false;
            if(Current_NextChannel1 == nullptr){
                Current_NextChannel1=NetChannelList.GetMyHead();
            }
            if(Current_NextChannel2 == nullptr) {
                Current_NextChannel2 = NetChannelList.GetNextNode(Current_NextChannel1);
            }
            if(Current_NextChannel3 == nullptr){
                Current_NextChannel3=NetChannelList.GetMyTail();
            }
        }
    }
    if(Current_NextChannel1!=nullptr&&Current_NextChannel2!=nullptr&&Current_NextChannel3!=nullptr){
        if(count<70){
            count++;
            return Current_NextChannel1;
        }
        else if(count<140){
            count++;
            return Current_NextChannel2;
        }
        else if(count<260){
            count++;
            return Current_NextChannel3;
        }
        else if(count==260){
            count=1;
            return Current_NextChannel1;
        }
    }
*/

    NetChannelList.Lock();
    for (auto node : NetChannelList.node_list) {
        if (Current_NextChannel == node) {
            // 找到匹配的通道
            Current_NextChannel = NetChannelList.GetNextNode(node);
            NetChannelList.UnLock();
            return node;
        }
    }
/*        else if(node->m_canWrite!=0){
            node->DecreaseCanWrite();
            Current_NextChannel = NetChannelList.GetNextNode(node);
        }*/
    NetChannelList.UnLock();
    return nullptr;
}

//230225
bool CNetTerminal::CreateNetChannel(DWORD nCID) {
    printf("terminal:CreateNetChannel.1\n");
    shared_ptr<CNetChannel> pNetChannel ;
    bool isDtuChannel = GetChannelDtuType(nCID);
    if(isDtuChannel == true) {
        pNetChannel = std::make_shared<CNetChannel>(nCID, true);
    }
    else{
        pNetChannel = std::make_shared<CNetChannel>(nCID);
    }

    NetChannelList.AddTail(std::move(pNetChannel));
    return true;
}

int CNetTerminal::AddNetChannel(std::shared_ptr<CNetChannel> pNetChannel) {
    //printf("[NCT::AutoTestDebug]::有新的通道添加到本终端%lu\n",m_dwTID);
    NetChannelList.AddTail(std::move(pNetChannel));
    return NetChannelList.GetMyCount();
}

DWORD CNetTerminal::RemoveNetChannel(const std::shared_ptr<CNetChannel> &pNetChannel) {
    DWORD nChannelNumber = -1;
    //若为已认证通道，则关闭掉通道类相关的UDP通道
    if (!pNetChannel->m_bIsTempChannel) {
        bool bOK = pNetChannel->CloseUDP();
    } else {
        //如果是临时通道，则将此临时通道归为已知通道
        pNetChannel->m_bIsTempChannel = false;
    }
    //从通道链表中移除此通道类
    NetChannelList.RemoveAt(pNetChannel);
    nChannelNumber = NetChannelList.GetMyCount();
    return nChannelNumber;
}

//驱动临时终端类中所有的通道号，进行状态流转
bool CNetTerminal::ChannelTraverse() {
    bool bOK = false;
    NetChannelList.Lock();//win版本没有加锁
    for (const auto &node: NetChannelList.node_list) {
        bOK = node->ChannelStateTraverse();
        if (!bOK) {
            NetChannelList.UnLock();
            return false;
        }
    }
    NetChannelList.UnLock();
    return true;
}

std::shared_ptr<CNetChannel> CNetTerminal::ChannelTraverse(DWORD nCID) {
    NetChannelList.Lock();
    for (auto node: NetChannelList.node_list) {
        if (nCID == node->m_dwCID) {
            NetChannelList.UnLock();
            return node;
        }
    }
    NetChannelList.UnLock();
    return nullptr;
}
