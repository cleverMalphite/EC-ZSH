//
// Created by 王炳棋 on 2022/11/20.
//

#ifndef NETCOMBTRANSFER_CNETTERMINALMANAGER_H
#define NETCOMBTRANSFER_CNETTERMINALMANAGER_H

#include "../mGlobalDef.h"
#include "CNetTerminal.h"
#include "CRouterInfo.h"
#include "CNetBuffer.h"
#include "CNetCombTransferBufferList.h"
#include "NetCombTransferCmdThread.h"
#include "NetCombTransferDataQueue.h"
#include "CIMsgRouterInfoNotify.h"
#include <unordered_map>
#include "NetCombTransferApi.h"
#include "./DTUVirtualServer.h"

#define MAX_TID_NUMBER 1000

struct TcpChannelDecode {
    //使用这个指针时应该注意，指针指向的内存块长度不确定
    std::shared_ptr<BYTE> m_pTempBuffer;    //TCP粘包数据缓存暂存指针
    DWORD m_dwTempLength;      //TCP粘包数据长度暂存指针变量
};

class CNetTerminalManager {
public:
    CNetTerminalManager(DWORD dwTID);

    virtual ~CNetTerminalManager();

private:
    /*bool m_bPauseThread;    //线程状态标识*/
public:
    //移植过程，线程的暂停尝试使用pthread_cond_wait和pthread_cond_signal实现
    //cond和mtx需要成对配合使用
#ifndef LinuxThread_suspend_and_resume
#define LinuxThread_suspend_and_resume
    pthread_mutex_t TempTermianlTraverse_mtx;
    pthread_cond_t TempTerminalTraverse_Cond;
    pthread_mutex_t NetCombTransferThread_mtx;
    pthread_cond_t NetCombTransferThread_Cond;
#endif
    std::shared_ptr<NetCombTransferCmdThread> m_pthread; //线程抽象类指针
    /*pthread_t m_nThreadID;  //线程ID*/
    DWORD m_dwTID;  //终端号
    DWORD m_d_self_tid = GetIntegerKeyIni("Main", "DeviceID", 100);
    bool m_bPauseThread;    //线程状态标识
    NetCombTransferDataQueue<CNetBuffer> m_DataList;
    NETCombTransferBufferList<CIMsg> m_MsgList;
    NETCombTransferBufferList<CNetBuffer> m_MsgBufferList;
    NETCombTransferBufferList<CNetTerminal> m_TerminalList;
    NETCombTransferBufferList<CRouterInfo> m_RouterInfoList;
    std::unordered_map<DWORD, std::shared_ptr<TcpChannelDecode>> m_tcpChannelDecodeHelp;
    CNetTerminal m_TempNetTerminal;
    pthread_mutex_t m_decode_lock;

#ifndef G_THREAD_NETCOMBTRANSFER
#define G_THREAD_NETCOMBTRANSFER
    /*void *m_thread_netcombtransfer = nullptr;*/
    pthread_t TraversePID = 0;
    pthread_t NetCombTransferPID = 0;
#endif

public:
    std::shared_ptr<DTUVirtualServer> m_pDTUVirtualServer;
public:
    pNetCombTransferMessageCallback m_pMessageCallBack[GLOBAL_MAX_REGISTER_CALLBACK_FUN];
    pNetCombTransferBufferCallback m_pDataCallBack[GLOBAL_MAX_REGISTER_CALLBACK_FUN];
    pNetMessageCallback m_pTerminalMessageCallBack;
private:
    const static DWORD ROUTE_CALLBACK_NUMBER_MAX = 10;
    DWORD ROUTE_CALLBACK_NUMBER = 0;
    pNetCombTransferRouterNotify m_pRouteCallBack[ROUTE_CALLBACK_NUMBER_MAX];

    CNetTerminalManager(const CNetTerminalManager &);

    const DWORD NETCOMM_MAX_UNDEAL_DATA_NUMBER = GetIntegerKeyIni("Main", "NETCOMM_MAX_UNDEAL_NUMBER", 20000);

public:
    void CallAllRouteCallBacks(DWORD dwSourceTID, DWORD dwDestinationTID, DWORD m_nNop,
                               DWORD m_nQosSend, DWORD m_nQosRecv, bool bSetted);

public:
    bool Init();

    bool UnInit();

    bool Notify();

    void Run();

public:
    bool IsRouterOn(DWORD dwDestinationTID);    //判断是否存在到目的终端的路由条目

    bool TmpTerminalStateTraverse();    //临时通道状态转换机

    bool DoRecvMsg(const std::shared_ptr<CNetBuffer> &pNetBuffer);

    bool DoRecvData(const std::shared_ptr<CNetBuffer> &pNetBuffer);

    bool DoRegister_TerminalCallBack(pNetMessageCallback p_Net_MessageCallback);  //注册终端回调函数

    bool DoRegister_NetCombTransferRouterCallBack(pNetCombTransferRouterNotify p_comb_transfer_router_notify);

    bool DoRegister_NetCombTransferCallBack(pNetCombTransferMessageCallback pMsgCallBack,
                                            pNetCombTransferBufferCallback pDataCallBack);

    DWORD FindNextHop(DWORD dwDestinationTID);

    bool DeleteRouterInfo(DWORD dwDestinationTID, DWORD dwSourceTID);

    bool SendRouterInfoToAllAndUpdate(DWORD dwSourceTID);

    bool SendRouterInfoToAll(DWORD dwExTID, DWORD dwDestinationTID);

    bool SendRouterInfoToAll(DWORD dwExTID, DWORD dwDestinationTID, DWORD nNop, DWORD m_nQosSend, DWORD m_nQosRecv);

    bool SendRouterInfo(DWORD dwDestination, DWORD dwCID);

    bool UpdateRouterInfo(DWORD dwDestinationTID, DWORD dwSourceTID, DWORD m_nNop, DWORD m_nQosSend, DWORD m_nQosRecv);

public:
    bool DoRecvCommand(const std::shared_ptr<CIMsg> &pMsg);

    bool DoIsRemoteTerminalOn(DWORD nRemoteTID);    //判断指定远程终端是否存在

    DWORD FindTIDByCID(DWORD nCID);

    bool SendData(DWORD dwDestinationTID, const std::shared_ptr<BYTE> &pBuffer, DWORD length, bool bBeTransmitted);

    bool SendMsg(DWORD TID, const std::shared_ptr<BYTE> &pBuffer, DWORD length, bool bBeTransmitted);

    std::shared_ptr<CNetTerminal> TerminalFind(DWORD nTID);

    std::shared_ptr<CNetTerminal> CreateNewTerminal(const std::shared_ptr<CNetChannel> &pNetChannel);

    bool MoveChannelToTerminal(const std::shared_ptr<CNetChannel> &pNetChannel);

    bool TerminalTraverse(DWORD nCID);

    bool SendCommand(const std::shared_ptr<CIMsg> &pMsg, DWORD nCID);

    bool ChannelArrived(DWORD nCID);

    bool ChannelLeaved(DWORD nCID);

    bool Recv_NetCommMessageNotify(DWORD nCID, bool bCreate);

    bool Recv_NetCommBufferNotify(DWORD dwCID, std::shared_ptr<BYTE> &pBuffer, DWORD nLength, bool isNeedSlit = false);

    bool Recv_NetQosMonitorMessageNotify(DWORD nCID, DWORD nNop, DWORD m_nQosSend, DWORD m_nQosRecv);

    bool TcpChannelActiveCloseByNetComm(DWORD nCID, bool bCreate);

    void terminalCallBack();  //将终端列表信息传递给terminal列表中，并调用回调函数向上层返回终端列表信息
};


#endif //NETCOMBTRANSFER_CNETTERMINALMANAGER_H
