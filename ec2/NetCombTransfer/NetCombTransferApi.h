//
// Created by 王炳棋 on 2022/11/19.
//

#ifndef NETCOMBTRANSFER_NETCOMBTRANSFERAPI_H
#define NETCOMBTRANSFER_NETCOMBTRANSFERAPI_H

#include "../mGlobalDef.h"
#include "NetCombTransferBase.h"
#include "IniHandleApi.h"

//------------------------终端结构定义-----------------------
typedef struct Terminal{
    DWORD TID;  //终端号
    unsigned int channelNum;  //lianlu shuliang
    unsigned int localPort;  //本地Port
    std::string localAddress;   //本地IP
    unsigned int remotePort;  //终端连接端口
    std::string remoteAddress;   //终端IP
    unsigned int state;   //终端连接状态
};

//------------------------回调函数接口定义----------------------------

//TODO:这里是修改 //通知上层新的终端列表
typedef void(*pNetMessageCallback)(
                  std::vector<Terminal> pTerminalList);
//通知上层设备号为TID的设备已经连接成功，可以进行通信了
typedef bool(*pNetCombTransferMessageCallback)(
                  DWORD dwCID, bool bCreate);
//将远端传来的数据通知给上层
typedef bool(*pNetCombTransferBufferCallback)(
                  DWORD dwTID, 
                  const std::shared_ptr<BYTE> &pBuffer, 
                  DWORD dwLength, bool bMsg, 
                  int64_t packet_recv_timeStamp);
//向上层通报与远程终端连接的通道质量
typedef void(*pNetCombTransferQosCallback)(
                  DWORD nTID,
                  DWORD dwChannelSendQuality, //向nTID发送数据的QoS质量
                  DWORD dwChannelRecvQuality );   //从nTID接收数据的QoS质量
//向上层通知路由条目更新
typedef void(*pNetCombTransferRouterNotify)(
                  DWORD dwSourceTID, 
                  DWORD dwDestinationTID, 
                  UINT m_nNop, UINT m_nQoSSend,
                  UINT m_nQosRecv, bool bSetted);


//-----------------------------------注册回调函数-----------------------


//数据和消息的回调函数注册（供SpeedControl用）
bool Register_NetCombTransferCallback(
              pNetCombTransferMessageCallback p_msg_callback,
              pNetCombTransferBufferCallback p_buffer_callback);

//路由更新的回调函数注册（供SpeedControl、MRUDP、RBUDP用）
bool Register_NetCombTransferRouterCallback(
              pNetCombTransferRouterNotify p_router_notify);

//注册终端列表回调函数(供界面用)
bool Register_TerminalListCallback(
              pNetMessageCallback p_net_message_callback);


//---------------------------------核心API函数------------------------
//初始化函数：初始化模块，在main函数开头用
bool Init_NetCombTransfer(DWORD dwTID);
//反初始化函数：初始化模块，在main函数结束用
bool UnInit_NetCombTransfer();
//发送函数：向指定终端发送数据（供SpeedControl用）
bool SendTIDData(DWORD TID, const std::shared_ptr<BYTE>& buffer, DWORD length, bool bBeTransmitted);
//发送函数：向指定终端发送数据（供SpeedControl用）
bool SendTIDCommand(DWORD TID, const std::shared_ptr<BYTE> &buffer, DWORD length);
//获取本地端口号
int Manager_GetLocalPort();


//-----------------------------------模块内部的全局函数-----------------------
//全局消息通知函数
bool globalNetCombTransferRecvMessage(DWORD nCID, bool bCreate);
//全局数据通知函数
bool
globalNetCombTransferRecvCommand(DWORD nCID, std::shared_ptr<BYTE> pBuffer, 
                                 DWORD nLength, bool isNeedSlit = false);
//全局命令发送函数
bool globalNetCombTransferSendCommand(const std::shared_ptr<CIMsg> &pMsg, DWORD nCID);
//全局函数，临时终端状态遍历
bool globalNetCombTransferTerminalStateTraverse();
//全局函数，打印所有的路由信息
void PrintAllRouters();
//NetComm反馈底层主动关闭了TCP连接
bool NetCombTransferGetTcpChannelClose(DWORD nCID, bool bCreate);
//test:遍历终端列表，看创建server或client时，是否加入终端列表
void traverseAllTerminals();

#endif //NETCOMBTRANSFER_NETCOMBTRANSFERAPI_H
