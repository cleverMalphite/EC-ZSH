//
// Created by 王炳棋 on 2022/11/20.
//

#include "DataCodec.h"
#include "CNetChannel.h"
#include "CNetTerminal.h"
#include "CIMsgShortMessage.h"
#include "CNetTerminalManager.h"
#include "CIMsgRemoteIDNotify.h"
#include "CIMsgRemoteIDRequest.h"
#include "CIMsgRouterInfoDeleteNotify.h"
#include "CIMsgChannelStatusNotify.h"
#include "CIMsgRemoteUDPPortNotify.h"
#include <utility>

#include "./NetCombTransferApi.h"

#include "infoHub/rateStatistic.h"


// ----------- infohub ------------
extern ec2::rateStatistic nct_rx_rate_stat; 
extern ec2::rateStatistic nct_msg_rx_rate_stat; 
extern ec2::rateStatistic nct_data_rx_rate_stat; 

#ifndef G_NETCOMBTRASFER_LOCK
#define G_NETCOMBTRASFER_LOCK

#endif
namespace NETCOMBTRANSFER {
}

//线程函数
/*void *ThreadProcForNetCombTransfer(void *argNeeded) {
    pthread_detach(pthread_self());
    for (;;) {
        try {
            globalNetCombTransferTerminalStateTraverse();
        } catch (...) {}
        usleep(NETCOMBTRANSFER_THREAD_DELAY_TIME * 1000);
    }
}*/

#ifdef LinuxThread_suspend_and_resume

void TraverseThreadCleanUp(void *arg) {
    //printf("\nTempTerminal Traverse线程的清理函数调用，线程关闭\n\n");
}

//测试通过，可以及时操作Manager的Cond_Signal唤醒此线程
void *ThreadProcForNetCombTransfer_Cond(void *argNeed) {
    auto pManager = (CNetTerminalManager *) argNeed;
    //printf("[NetCombTransfer]::TempTerminal Traverse Thread Created\n");
    pthread_cleanup_push(TraverseThreadCleanUp, nullptr);
        for (;;) {
            /*自定义取消点*/
            pthread_testcancel();
            pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, nullptr);
            pthread_mutex_lock(&pManager->TempTermianlTraverse_mtx);
            try {
                if (pManager->m_bPauseThread) {
                    //printf("[NetCombTransfer]::TempTerminal TraverseThread has suspended\n");
                    /*If successful, the pthread_cond_wait() function will return zero.
                     * Otherwise, an error number will be returned to indicate the error.*/
                    pthread_cond_wait(&pManager->TempTerminalTraverse_Cond, &pManager->TempTermianlTraverse_mtx);
                }
                globalNetCombTransferTerminalStateTraverse();
            } catch (...) {}
            usleep(NETCOMBTRANSFER_THREAD_DELAY_TIME * 1000);
            pthread_mutex_unlock(&pManager->TempTermianlTraverse_mtx);
            pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);
        }
    pthread_cleanup_pop(0);
}

#endif

//注册到NetComm中的消息回调函数
bool OnMESSAGECALLBACK(DWORD nCID, bool bCreate) {
    /**
     * 将本终端与外部系统终端进行通信的通道屏蔽掉
     */
    printf("TErminalMAnager:OnMESSAGECALLBACK.1\n");
    //先判断通道号是否在预留给ThirdPartyTransfer的号段内
    if (RESERVE_CHANNEL_BEGIN <= nCID && nCID <= RESERVE_CHANNEL_END) {
        printf("TErminalMAnager:OnMESSAGECALLBACK.2\n");
        //如果在预留号段内，那么直接返回
        return true;
    }
    bool bOK = false;
    try {
        printf("TErminalMAnager:OnMESSAGECALLBACK.3\n");
        bOK = globalNetCombTransferRecvMessage(nCID, bCreate);
    }
    catch (...) {
        //防止因为异常导致没有退出临界区
    }
    return bOK;
}

bool ClientActiveShutDownCALLBACK(DWORD nCID, bool bCreate) {
    /**
     * 用于本地主动关闭TCP通道将相应的UDP通道关闭
     */
    if (RESERVE_CHANNEL_BEGIN <= nCID && nCID <= RESERVE_CHANNEL_END) {
        //如果在预留号段内，那么直接返回
        return true;
    }
    bool bOK = false;
    try {
        bOK = NetCombTransferGetTcpChannelClose(nCID, bCreate);
    }
    catch (...) {
        //防止因为异常导致没有退出临界区
    }
    return bOK;
}

//注册到NetComm中的数据回调函数
bool OnDATACALLBACK(DWORD nChannel, const std::shared_ptr<BYTE> &pBuffer, DWORD nLength, bool isNeedSlit) {
//    printf("[CNETTerminalManager] ONDATACALLBACK\n");

    /**
     * 将本终端与外部系统终端进行通信的通道屏蔽掉
     */
    //先判断通道号是否在预留给ThirdPartyTransfer的号段内
    if (RESERVE_CHANNEL_BEGIN <= nChannel && nChannel <= RESERVE_CHANNEL_END) {
        //如果在预留号段内，则直接返回
        return true;
    }
    //复制数据
    std::shared_ptr<BYTE> pBufferCopy(new BYTE[nLength], releaseArrays<BYTE>);
    //win版本下采用了memcpy_s，但没有使用其返回值，linux下没有memcpy_s函数
    //如果目标区域和源区域有重叠的话,memcpy会出现错误,可以采用memmove避免掉这个错误
    memcpy(pBufferCopy.get(), pBuffer.get(), nLength);

    bool bOK = false;
    //进入临界区操作？原码此处注释了临界区操作，移植后需要判断是否需要加锁/临界区
    try {
        bOK = globalNetCombTransferRecvCommand(nChannel, pBufferCopy, nLength, isNeedSlit);
    } catch (...) {
        //离开临界区
    }
    //离开临界区
    return bOK;
}

CNetTerminalManager::CNetTerminalManager(DWORD dwTID) {
    printf("[DEBUG] CNetTerminalManager() Loacl TID:%d\n", dwTID);
    m_dwTID = dwTID;
    m_bPauseThread = true; //表示现在还没开启线程
    TraversePID = 0;
    NetCombTransferPID = 0;
    pthread_mutex_init(&m_decode_lock, nullptr);
#ifdef LinuxThread_suspend_and_resume
    TempTermianlTraverse_mtx = PTHREAD_MUTEX_INITIALIZER;
    TempTerminalTraverse_Cond = PTHREAD_COND_INITIALIZER;
    NetCombTransferThread_mtx = PTHREAD_MUTEX_INITIALIZER;
    NetCombTransferThread_Cond = PTHREAD_COND_INITIALIZER;
#endif
    //初始化其它回调指针为空，这里一定要注意是从一开始的，因为第0个回调函数是给上层模块留的
    for (int i = 0; i < GLOBAL_MAX_REGISTER_CALLBACK_FUN; ++i) {
        m_pMessageCallBack[i] = nullptr;
        m_pDataCallBack[i] = nullptr;
    }
    m_pTerminalMessageCallBack = nullptr;
    //printf("[NetComm]gNetCombTransferManager generate\n");
}

CNetTerminalManager::~CNetTerminalManager() {
    //1.清理消息列表
    m_MsgList.node_list.clear();
    //2. 清理路由消息列表
    m_RouterInfoList.node_list.clear();
    //3. 清理终端记录列表
    m_TerminalList.node_list.clear();
    pthread_mutex_destroy(&m_decode_lock);
#ifdef LinuxThread_suspend_and_resume
    pthread_mutex_destroy(&TempTermianlTraverse_mtx);
    pthread_cond_destroy(&TempTerminalTraverse_Cond);
    pthread_mutex_destroy(&NetCombTransferThread_mtx);
    pthread_cond_destroy(&NetCombTransferThread_Cond);
#endif
}

//调用所有的路由回调
void
CNetTerminalManager::CallAllRouteCallBacks(DWORD dwSourceTID, DWORD dwDestinationTID, DWORD m_nNop, DWORD m_nQosSend,
                                           DWORD m_nQosRecv, bool bSetted) {
    for (DWORD i = 0; i < ROUTE_CALLBACK_NUMBER; i++) {
        if (m_pRouteCallBack[i]) {
            m_pRouteCallBack[i](dwSourceTID, dwDestinationTID, m_nNop, m_nQosSend, m_nQosRecv, bSetted);
        }
    }
}

//todo:needTest,2022/11/21
//linux版本的NetCombTransfer初始化函数，使用了cond_wait和cond_signal来实现线程的挂起和唤醒
bool CNetTerminalManager::Init() {
    bool bOK = false;
    //初始化
    //
    
    m_pDTUVirtualServer = std::make_shared<DTUVirtualServer>();
    if (nullptr == m_pDTUVirtualServer) {
        //说明初始化失败了，就直接返回false
    } else {
        printf("DTU virtual server init ok!!!\n");
    }

    for (int i = 0; i < NETCOMBTRANSFER_MAX_INIT_TIMES; ++i) {
        printf("terminalManager:init\n");
        bOK = Init_NetComm(OnMESSAGECALLBACK, ClientActiveShutDownCALLBACK, OnDATACALLBACK);
        if (!bOK) break;
        m_pthread = std::make_shared<NetCombTransferCmdThread>(this);
        if (m_pthread) {
            m_pthread->startup();   //启动CombTransfer的工作线程
        }
        if (0 == TraversePID) {
            //开启一个临时终端的通道流转线程
            pthread_create(&TraversePID, nullptr, ThreadProcForNetCombTransfer_Cond, this);
        }
        usleep(500 * 1000);
        if (0 != m_pthread->m_hPlayThread) {
            NetCombTransferPID = m_pthread->m_hPlayThread;
        }
        //起初创建的线程应当是挂起的,此时激活线程;
        m_bPauseThread = false;
        /*If successful, the pthread_cond_signal() function will return zero,
         * otherwise an error number will be returned to indicate the error.*/
        int WakeupRet1 = pthread_cond_signal(&NetCombTransferThread_Cond);
        int WakeupRet2 = pthread_cond_signal(&TempTerminalTraverse_Cond);
        if (WakeupRet1 || WakeupRet2) {
            //说明线程唤醒失败
            return false;
        }
        break;
    }
    return bOK;
}

bool CNetTerminalManager::UnInit() {
    bool bOK = false;
    //关闭Traverse线程？
    if (0 != TraversePID) {
        pthread_cancel(TraversePID);
        pthread_join(TraversePID, nullptr);
        TraversePID = 0;
    }
    m_bPauseThread = true;

    if (m_pthread) {
        m_pthread->cancel();
        m_pthread = nullptr;
    }
    //注销NetComm模块
    //printf("\n开始注销NetComm模块...\n\n");
    bOK = UnInit_NetComm();

    //printf("[NetComm]:模块注销完成...\n");

    return bOK;
}

DWORD CNetTerminalManager::FindTIDByCID(DWORD nCID) {
    m_TerminalList.Lock();
    for (const auto &pTerminal: m_TerminalList.node_list) {
        std::shared_ptr<CNetChannel> pNetChannel = pTerminal->ChannelTraverse(nCID);
        if (pNetChannel && pNetChannel.get()) {
            m_TerminalList.UnLock();
            return pNetChannel->m_dwTID;
        }
    }
    m_TerminalList.UnLock();
    return -1;
}

//NetCombTransfer的运转代码
void CNetTerminalManager::Run() {
    const static int RUN_TIMES_COUNT_MAX = 5000;
    try {
        if (m_bPauseThread) {
            return;
        }
        //定义并初始化一个变量，表示线程是否正处理命令
        bool bDealProcess = true;
        unsigned ProcessCounts = 0;
        while (bDealProcess && ProcessCounts < RUN_TIMES_COUNT_MAX) {
            bDealProcess = false;
            //取出命令并执行
            std::shared_ptr<CIMsg> pMsg = m_MsgList.RemoveHead();
            if (pMsg && pMsg.get()) {
                DoRecvCommand(pMsg);
                pMsg = nullptr;
                bDealProcess = true;
            }
            //取出消息并处理
            std::shared_ptr<CNetBuffer> pNetMsgBuffer = m_MsgBufferList.RemoveHead();
            if (pNetMsgBuffer && pNetMsgBuffer.get()) {
                DoRecvMsg(pNetMsgBuffer);
                pNetMsgBuffer = nullptr;
                bDealProcess = true;
            }
            //处理数据
            std::shared_ptr<CNetBuffer> pNetDataBuffer = m_DataList.RemoveHead();
            if (pNetDataBuffer && pNetDataBuffer.get()) {
                DoRecvData(pNetDataBuffer);
                pNetDataBuffer = nullptr;
                bDealProcess = true;
            }
            ProcessCounts++;
        }
        if (!bDealProcess) {
            //如果这轮什么也没有处理则休眠线程1ms
            usleep(1000);
        }
    } catch (...) {
        return;
    }
}

/*
 * 这个方法用于：再有终端上线时，调用所有的相应回调函数通知上层
 * */
bool CNetTerminalManager::Notify() {
    bool bOK = false;
    //回调函数[0]在何时设置？作何用处？
    if (m_pMessageCallBack) {
        for (int i = 0; i < GLOBAL_MAX_REGISTER_CALLBACK_FUN; ++i) {
            if (nullptr != m_pMessageCallBack[i]) {
                bOK = m_pMessageCallBack[i](m_dwTID, true);
                if (!bOK) {
                    return bOK;
                }
            }
        }
    }
    return bOK;
}

bool CNetTerminalManager::DoRecvCommand(const std::shared_ptr<CIMsg> &pMsg) {
    bool bOK = false;
    if (pMsg) {
        DWORD dwCommand = pMsg->GetCommandID();//获取指令类型
        DWORD dwCID = pMsg->m_dwDeviceID;//获取消息来源通道号
        if (dwCommand == eCommandShortMessage) {
            std::shared_ptr<CIMsgShortMessage> pShortMsg = std::dynamic_pointer_cast<CIMsgShortMessage>(pMsg);
            /*printf("[NetCombTransferDebug]:接收到ShortMessage,{%d}-->{%d},DataLength is %d / %d\n",
                   pShortMsg->GetSendTID(), pShortMsg->GetRecvTID(), pShortMsg->GetLength(),
                   pShortMsg->GetTotalLength());*/
    
            //printf("[DEBUG]CNetTerminalManager::DoRecvCommand::pMsg->GetRecvTID:%d\n",
            //       pShortMsg->GetRecvTID());

            if (m_d_self_tid == pShortMsg->GetRecvTID()) {
                for (int i = 0; i < GLOBAL_MAX_REGISTER_CALLBACK_FUN; ++i) {
                    if (nullptr != m_pDataCallBack[i]) {
                        //printf("CommandPacket Timestamp is %ld\n", pShortMsg->Getpacket_decode_TimeStamp());
                        m_pDataCallBack[i](pShortMsg->GetSendTID(), pShortMsg->GetDataBuffer(), pShortMsg->GetLength(),
                                           true, pShortMsg->Getpacket_decode_TimeStamp());
                    }
                }
            } else {
                bOK = SendMsg(pShortMsg->GetRecvTID(), pShortMsg->GetTotalData(), pShortMsg->GetTotalLength(), true);
            }
            return true;
        }
        {   //找到消息来源对应的通道类，目前不需要考虑已认证终端发来的这类消息，所以从临时终端中查找
            std::shared_ptr<CNetChannel> pNetChannel = m_TempNetTerminal.ChannelTraverse(dwCID);
            if (pNetChannel && pNetChannel.get()) {
                DWORD nChannelState = pNetChannel->GetChannelState();
                if (E_NETCOMB_STATUS_RUDPPORT_CONFIRMED == nChannelState) {
                    return true;
                }
            }
        }
        switch (dwCommand) {
            //如果是RID请求命令，则发送一个RID通知
            case eCommandRemoteIDRequest: {
                std::shared_ptr<CIMsgRemoteIDNotify> RIDNotifyMsg = std::make_shared<CIMsgRemoteIDNotify>();
                //为此指令设置上终端号
                RIDNotifyMsg->m_dwTID = m_dwTID;
                bOK = SendCommand(RIDNotifyMsg, dwCID);
                //根据bOK打log
                printf("[DoRecvCommand]get Command remote ID request, and send CID:%d, TID:%d", 
                       dwCID, m_dwTID);
            }
                break;
            case eCommandRemoteIDNotify: {

                std::shared_ptr<CIMsgRemoteIDNotify> RIDNotifyMsg = std::dynamic_pointer_cast<CIMsgRemoteIDNotify>(
                        pMsg);


                //读出发送此指令的终端号
                DWORD dwTID = RIDNotifyMsg->m_dwTID;

                printf("[DoRecvCommand]get Command remote id notify, and TID:%d", 
                       dwTID);

                std::shared_ptr<CNetTerminal> pNetTerminal = TerminalFind(dwTID);


                //如果找到了，证明此终端已经被认证，不需要进行后续处理了，如果没找到则:
                if (!pNetTerminal) {
                    //既然可以收到此终端的RID消息，说明终端间已经建立了TCP连接
                    //那么就需要从未认证TCP通道中检查出对应于m_dwDeceiveID的消息
                    std::shared_ptr<CNetChannel> pNetChannel = m_TempNetTerminal.ChannelTraverse(dwCID);
                    if (pNetChannel) {
                        //mark:这里的RIDNotify终端号传入值是否出错？没有错，标识了通道所属的终端号
                        pNetChannel->RIDNotify(dwTID);//我们得知了对端的终端号，应该让对端也得知我们的终端号(ps:怎么让对端得知的？
                        bOK = pNetChannel->RUDPPortRequest();//得知对端TID后下一步只需要知道对端的UDP端口号
                    }
                } else{
                    std::shared_ptr<CNetChannel> pNetChannel = m_TempNetTerminal.ChannelTraverse(dwCID);
                    if (pNetChannel) {
                        //mark:这里的RIDNotify终端号传入值是否出错？没有错，标识了通道所属的终端号
                        pNetChannel->RIDNotify(dwTID);//我们得知了对端的终端号，应该让对端也得知我们的终端号(ps:怎么让对端得知的？
                        bOK = pNetChannel->RUDPPortRequest();//得知对端TID后下一步只需要知道对端的UDP端口号
                    }
                    printf("pass TIDNotify\n");
                }
            }
                break;
            case eCommandRemoteUDPPortRequest: {

                {   //若收到请求的通道是临时通道
                    std::shared_ptr<CNetChannel> pNetChannel = m_TempNetTerminal.ChannelTraverse(dwCID);
                    if (pNetChannel) {
                        std::shared_ptr<CIMsgRemoteUDPPortNotify> PortNotifyMsg = std::make_shared<CIMsgRemoteUDPPortNotify>();

                        PortNotifyMsg->m_nUDPPort = pNetChannel->m_nLocalUDPPort;

                        bool isDtuChannel = GetChannelDtuType(dwCID);
                        if(isDtuChannel == true){
                            //PortNotifyMsg->m_nUDPPort = gDTUVirtualServer->GetNatPort( pNetChannel->m_nLocalUDPPort );
                            PortNotifyMsg->m_nUDPPort = m_pDTUVirtualServer->GetNatPort( pNetChannel->m_nLocalUDPPort );
                            
                            printf("\n<<[2]>>\n");
                            printf("DoRecvCommand-->eCommandRemoteUDPPortRequest-->m_nUDPPort:\n");
                            printf("-------------------->%d:\n",PortNotifyMsg->m_nUDPPort );
                            printf("=====================================================\n");

                            if(PortNotifyMsg->m_nUDPPort == -1){
                                return false;
                            }
                        }
                        else{
                            PortNotifyMsg->m_nUDPPort = pNetChannel->m_nLocalUDPPort;
                        }

                        printf("\n<<[1]>>\n");
                        printf("DoRecvCommand-->eCommandRemoteUDPPortRequest-->m_nUDPPort:\n");
                        printf("-------------------->%d:\n",PortNotifyMsg->m_nUDPPort );
                        printf("=====================================================\n");

                        bOK = SendCommand(PortNotifyMsg, dwCID);
                        if (bOK) {

                        } else {

                        }
                        return bOK;
                    }
                }
                //若收到请求单通道是已经认证的通道
                m_TerminalList.Lock();
                for (const auto &pTerminal: m_TerminalList.node_list) {
                    std::shared_ptr<CNetChannel> pNetChannel = pTerminal->ChannelTraverse(dwCID);
                    if (pNetChannel) {
                        std::shared_ptr<CIMsgRemoteUDPPortNotify> PortNotifyMsg = std::make_shared<CIMsgRemoteUDPPortNotify>();
                        bool isDtuChannel = GetChannelDtuType(dwCID);
                        if(isDtuChannel == true){
                            //PortNotifyMsg->m_nUDPPort = gDTUVirtualServer->GetNatPort( pNetChannel->m_nLocalUDPPort );
                            PortNotifyMsg->m_nUDPPort = m_pDTUVirtualServer->GetNatPort( pNetChannel->m_nLocalUDPPort );
                            
                            printf("\n<<[2]>>\n");
                            printf("DoRecvCommand-->eCommandRemoteUDPPortRequest-->m_nUDPPort:\n");
                            printf("-------------------->%d:\n",PortNotifyMsg->m_nUDPPort );
                            printf("=====================================================\n");

                            if(PortNotifyMsg->m_nUDPPort == -1){
                                return false;
                            }
                        }
                        else{
                            PortNotifyMsg->m_nUDPPort = pNetChannel->m_nLocalUDPPort;
                        }

                        bOK = SendCommand(PortNotifyMsg, dwCID);
                        m_TerminalList.UnLock();
                        return bOK;
                    }
                }
                m_TerminalList.UnLock();
            }
                break;
                //收到通道对端UDP端口号和网卡信息
            case eCommandRemoteUDPPortNotify: {
                std::shared_ptr<CIMsgRemoteUDPPortNotify> PortNotifyMsg = std::dynamic_pointer_cast<CIMsgRemoteUDPPortNotify>(
                        pMsg);
                //获取对端UDP端口号
                DWORD nRemotePort = PortNotifyMsg->m_nUDPPort;
                //找到消息来源对应的通道类，目前不需要考虑已认证终端发来的这类消息
                std::shared_ptr<CNetChannel> pNetChannel = m_TempNetTerminal.ChannelTraverse(dwCID);
                if (pNetChannel) {
                    //应当确保状态流转准确
                    DWORD nChannelState = pNetChannel->GetChannelState();
                    if (E_NETCOMB_STATUS_CHANNEL_RID_CONFIRMED == nChannelState) {

                       printf("\n<<[eCommandRemoteUDPPortNotify]>>\n");
                       printf("------->%d:\n", nRemotePort);
                       printf("=====================================================\n");
                        

                        //如果对端TID已经获取
                        pNetChannel->RUDPPortNotify(nRemotePort);
                        //将通道移入对应的终端
                        bOK = MoveChannelToTerminal(pNetChannel);
                        if (bOK) {
#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
                            //获取通道所属的TID
                            DWORD dwDestinationTID = pNetChannel->m_dwTID;
                            //通知对端本地已有的路由信息
                            //SendRouterInfo只在此处调用,有新通道加入到终端中
                            bOK = SendRouterInfo(dwDestinationTID, dwCID);
                            if (bOK) {
                                //
                                bOK = SendRouterInfoToAll(dwDestinationTID, dwDestinationTID, 0, 0, 0);
                            }
                        } else {
#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
                        }
                    }
                }
            }
                break;
                //收到对端路由增添更新通知
            case eCommandRouterInfoNotify: {
                //解析路由信息
                std::shared_ptr<CIMsgRouterInfoNotify> RouterMsg = std::dynamic_pointer_cast<CIMsgRouterInfoNotify>(
                        pMsg);
                DWORD dwDestinationTID = RouterMsg->m_dwDestinationTID;
                DWORD dwSourceTID = RouterMsg->m_dwSourceTID;
                DWORD nNop = RouterMsg->m_nNop;
                DWORD nQosSend = RouterMsg->m_nQosSend;
                DWORD nQosRecv = RouterMsg->m_nQosRecv;

                //更新路由表信息
#ifdef COMBTRANSFER_ROUTERINFO
#endif
                bOK = UpdateRouterInfo(dwDestinationTID, dwSourceTID, nNop, nQosSend, nQosRecv);
                if (bOK) {
                    //向邻居发送路由信息
                    bOK = SendRouterInfoToAll(dwSourceTID, dwDestinationTID, nNop, nQosSend, nQosRecv);
                }
            }
                break;
                //收到对端路由删除通知
            case eCommandRouterInfoDeleteNotify: {
                //解析得到的路由删减通知信息

                std::shared_ptr<CIMsgRouterInfoDeleteNotify> RouterMsg = std::dynamic_pointer_cast<CIMsgRouterInfoDeleteNotify>(
                        pMsg);
                DWORD dwDestinationTID = RouterMsg->m_dwDestinationTID;
                DWORD dwSourceTID = RouterMsg->m_dwSourceTID;
                //删除路由信息
                bOK = DeleteRouterInfo(dwDestinationTID, dwSourceTID);
                if (bOK) {

                    //这里是i++
                    for (int i = 0; i < GLOBAL_MAX_REGISTER_CALLBACK_FUN; i++) {
                        if (nullptr != m_pMessageCallBack[i]) {
                            m_pMessageCallBack[i](dwDestinationTID, false);
                        }
                    }
                    bOK = SendRouterInfoToAll(dwSourceTID, dwDestinationTID);
                }
            }
                break;
            default:
                break;
        }
    }
    return bOK;
}

bool CNetTerminalManager::DoRecvData(const std::shared_ptr<CNetBuffer> &pNetBuffer) {
    bool bOK = false;
    printf("[NetCombTransfer] Recv Data!!!\n");
    //解码数据
    std::shared_ptr<BYTE> &pBuffer = pNetBuffer->m_pBuffer;
    DWORD nLength = pNetBuffer->m_dwLength;



    int64_t data_recv_timestamp = pNetBuffer->m_data_recv_timeStamp;
    printf("DataPacket Timestamp is %ld\n", data_recv_timestamp);

    DWORD dwDestinationTID = 0;
    DWORD dwSourceTID = 0;
    bOK = DataCodec::GetRouterInfo(pBuffer, nLength, dwSourceTID, dwDestinationTID);

    //printf("[DEBUG]::CNetTerminalManager::DoRecvData sTID:%d, dTID:%d\n", 
    //       dwSourceTID, dwDestinationTID);


    printf(">>>[DEBUG]::CNetTerminalManager::DoRecvData sTID:%d, dTID:%d\n",
           dwSourceTID, dwDestinationTID);

    if (bOK) {
        //无需转发的情况
        if (m_dwTID == dwDestinationTID) {
            bOK = IsRouterOn(dwSourceTID);//判断此时发送这条消息的终端是否在线
            if (!bOK) {
                //不在线则退出
                return bOK;
            }

            //TEST:测试视频传输
            //fwrite(pBuffer.get(),nLength,1,fp_DataCodec_recv);

            DWORD nOutLength = 0;
            std::shared_ptr<BYTE> pDecodeBuffer = \
                                  DataCodec::DecodeData(
                                            pBuffer, 
                                            nLength,
                                            dwSourceTID,
                                            dwDestinationTID,
                                            nOutLength);

            //-----infohub ---------
            nct_rx_rate_stat.pass(nLength);
            nct_data_rx_rate_stat.pass(nLength); 

            //开始调用数据回调函数
            for (int i = 0; i < GLOBAL_MAX_REGISTER_CALLBACK_FUN; ++i) {
                if (nullptr != m_pDataCallBack[i]) {
                    printf("[NetCombTransfer]::DecodeData的首字节是:%d~~~\n", pDecodeBuffer.get()[0]);
                    m_pDataCallBack[i](dwSourceTID, pDecodeBuffer, nOutLength, false, data_recv_timestamp);
                }
            }
            //pDecodeBuffer = nullptr;
        } else {
            //需要转发
            bOK = SendData(dwDestinationTID, pBuffer, nLength, true);
        }
    }
    return bOK;
}

bool CNetTerminalManager::DoRecvMsg(const std::shared_ptr<CNetBuffer> &pNetBuffer) {
    bool bOK = false;
//    printf("[NetCombTransfer]::Recv Massage!!!\n");
    std::shared_ptr<BYTE> pBuffer = pNetBuffer->m_pBuffer;
    DWORD nLength = pNetBuffer->m_dwLength;

    DWORD dwDestinationTID = 0;
    DWORD dwSourceTID = 0;

    bOK = DataCodec::GetRouterInfo(pBuffer, nLength, dwSourceTID, dwDestinationTID);

  
    //printf("[DEBUG]::CNetTerminalManager::DoRecvMsg sTID:%d, dTID:%d\n", 
    //       dwSourceTID, dwDestinationTID);

    if (bOK) {
        if (m_dwTID == dwDestinationTID) {
            DWORD nOutLength = 0;
            std::shared_ptr<BYTE> pDecodeBuffer = DataCodec::DecodeMsg(pBuffer, nLength, dwSourceTID, dwDestinationTID,
                                                                       nOutLength);
            //-----infohub ---------
            nct_rx_rate_stat.pass(nLength);
            nct_msg_rx_rate_stat.pass(nLength); 

            //数据回调
            for (int i = 0; i < GLOBAL_MAX_REGISTER_CALLBACK_FUN; ++i) {
                if (nullptr != m_pDataCallBack[i]) {
                    //printf("[NetCombTransfer]::DecodeMassage的首字节是:%d~~~\n", pDecodeBuffer.get()[0]);
                    m_pDataCallBack[i](dwSourceTID, pDecodeBuffer, nLength, true, 0);
                }
            }
            pDecodeBuffer = nullptr;
        } else {
            bOK = SendMsg(dwDestinationTID, pBuffer, nLength, true);
        }
    }
    return bOK;
}


bool CNetTerminalManager::DoIsRemoteTerminalOn(DWORD nRemoteTID) {
    bool bOK = false;
    m_TerminalList.Lock();
    try {
        for (const auto &pTerminal: m_TerminalList.node_list) {
            if (nRemoteTID == pTerminal->m_dwTID) {
                bOK = true;
                break;
            }
        }
        m_TerminalList.UnLock();
        return bOK;
    } catch (...) {
        m_TerminalList.UnLock();
        return false;
    }
}

//遍历各个终端（已认证的和没有认证的）删除对应的通道，在必要时也会删除终端
bool CNetTerminalManager::TerminalTraverse(DWORD nCID) {
    printf("terminalManager:TERminalTraverse.1\n");
    bool bOK = false;
    {   //从临时终端中找此通道，并删除；
        std::shared_ptr<CNetChannel> pNetChannel = m_TempNetTerminal.ChannelTraverse(nCID);
        if (pNetChannel) {
            //由于我们统一使用m_TempNetTerminal这一实例来管理所有未认证的通道，所以不能删除这个实例
            m_TempNetTerminal.RemoveNetChannel(pNetChannel);
            return true;
        }
    }
    //mark20230327
    for (const auto pTerminal: m_TerminalList.node_list) {
        std::shared_ptr<CNetChannel> pNetChannel = pTerminal->ChannelTraverse(nCID);
        if (pNetChannel && pNetChannel.get()) {
            //获取剩余通道数，判断对端是否完全离开
            DWORD nChannelNum = pTerminal->RemoveNetChannel(pNetChannel);
            if (0 == nChannelNum) {
                //如果对端完全离开了，从终端链表移除该远程终端
                m_TerminalList.RemoveAt(pTerminal);

                //TODO:这里是修改的地方begin
//                printf("terminalManager:TERminalTraverse.2\n");
                terminalCallBack();
                //TODO:这里是修改的地方end

                //获取远程终端的终端TID
                DWORD dwRemoteTID = pTerminal->m_dwTID;
                //更新路由表，删除过时信息
                bool Ret1 = DeleteRouterInfo(dwRemoteTID, m_dwTID);
                if (Ret1) {
                    SendRouterInfoToAll(dwRemoteTID, dwRemoteTID);
                }
                bool Ret2 = SendRouterInfoToAllAndUpdate(dwRemoteTID);
                bOK = Ret1 & Ret2;
                if (bOK)
                    /*if (m_pMessageCallBack[0]) {*/
                    for (auto &MsgCallback: m_pMessageCallBack) {
                        if (MsgCallback) {
                            bOK = MsgCallback(dwRemoteTID, false);
                            if (!bOK) {
                                return bOK;
                            }
                        }
                    }
                /*}*/
            } else {
                //如果远程终端并没有断开所有通道，那么只需要重排该终端的通道值
                /*pTerminal->ReloadNetChannel(nCID);*/
                //TODO:这里是修改的地方begin
                terminalCallBack();
                //TODO:这里是修改的地方end
                //现在考虑不进行重拍，因为底层做出了调整
                bOK = true;
            }
            break;
        }
    }
    return bOK;
}

//向远端发送指令，通过TCP通道发送
bool CNetTerminalManager::SendCommand(const std::shared_ptr<CIMsg> &pMsg, DWORD nCID) {
    try {
        bool bOK = false;

        if (pMsg) {
            DWORD nLength = 0;
            std::shared_ptr<BYTE> pBuffer = pMsg->Encode(nLength);

            if (pBuffer) {
                bOK = WriteTcpChannel(nCID, pBuffer, nLength);
                pBuffer = nullptr;
            }
        }

        return bOK;
    }
    catch (...) {
        return false;
    }
}

//向目标终端发送数据
bool CNetTerminalManager::SendData(DWORD dwDestinationTID, const std::shared_ptr<BYTE> &pBuffer, DWORD length,
                                   bool bBeTransmitted) {

//    printf("<!!!> CNetTerminalManager:: SendData, dTID:%d\n", dwDestinationTID);

    bool bOK = false;
    //获取下一条TID
    DWORD dwNextHopTID = FindNextHop(dwDestinationTID);

//    printf("<!!!> CNetTerminalManager:: SendData, NextHopTID:%d\n", dwNextHopTID);


    //找到相应的路由条目
    if (-1 != dwNextHopTID) {
        //找到对应的终端实例
        std::shared_ptr<CNetTerminal> pNetTerminal = TerminalFind(dwNextHopTID);
        if (pNetTerminal && pNetTerminal.get()) {
            std::shared_ptr<CNetChannel> pNetChannel = pNetTerminal->ChannelSelect();
/*            if(!pNetChannel){
                pNetChannel=pNetTerminal->ChannelSelect();
            }*/
            if (pNetChannel && pNetChannel.get()) {
                //先判断数据是不是被转发，如果不是被转发，那么说明路由信息中源终端和目的终端还没有被加上
                if (!bBeTransmitted) {
                    DWORD nOutLength = 0;
                    std::shared_ptr<BYTE> pSendBuffer = DataCodec::EncodeData(pBuffer, length, m_dwTID,
                                                                              dwDestinationTID, nOutLength);

                    //TEST:测试视频传输
//                    fwrite(pSendBuffer.get(),nOutLength,1,fp_DataCodec_send);

                    if (pSendBuffer && nOutLength) {
                        bOK = pNetChannel->SendData(pSendBuffer, nOutLength);
                        pNetChannel->UpdateChannelStatus(bOK);
                    }
                    pSendBuffer = nullptr;
                } else {
                    //如果数据本身就是被转发的数据，那么命令数据标志位、源终端和目的终端都应该已经加上了
                    bOK = pNetChannel->SendData(pBuffer, length);
                }
            } else {
//                printf("没有找到合适的通道\n");
            }
        }
    }
    return bOK;
}

//向目标终端发送命令
bool
CNetTerminalManager::SendMsg(DWORD dwDestinationTID, const std::shared_ptr<BYTE> &pBuffer, DWORD length,
                             bool bBeTransmitted) {
    bool bOK = false;
    DWORD dwNextHopTID = FindNextHop(dwDestinationTID);
    /*printf("[NetCombTransferDebug]::发送目的端是%d的command,找到下一跳地址是:%d,这条消息是否为转发的?%d\n", dwDestinationTID, dwNextHopTID,
           bBeTransmitted);*/
    if (-1 != dwNextHopTID) {
        std::shared_ptr<CNetTerminal> pNetTerminal = TerminalFind(dwNextHopTID);
        if (pNetTerminal && pNetTerminal.get()) {
            std::shared_ptr<CNetChannel> pNetChannel = pNetTerminal->ChannelSelect();
            if (pNetChannel && pNetChannel.get()) {
                //先判断数据是不是被转发，如果不是被转发，那么说明路由信息中源终端和目的终端还没有被加上
                if (!bBeTransmitted) {
                    //MARK:23/1/6
                    //[NetCombTransfer]::m_d_self_tid
                    DWORD nOutLength = 0;
                    std::shared_ptr<CIMsgShortMessage> pMsg =
                            std::make_shared<CIMsgShortMessage>(/*m_d_self_tid*/m_dwTID, dwDestinationTID, length,
                                                                                pBuffer);
                    std::shared_ptr<BYTE> pSendBuffer = pMsg->Encode(nOutLength);
                    if (pSendBuffer && nOutLength) {
                        bOK = pNetChannel->SendCommand(pSendBuffer, nOutLength);
                    }
                    pSendBuffer = nullptr;
                } else {
                    //如果数据本身就是被转发的数据，那么命令数据标志位、源终端和目的终端都应该已经加上了
                    bOK = pNetChannel->SendCommand(pBuffer, length);
                }
            }
        }
    }
    return bOK;
}

/**
 * 当有新的通道连接到本终端时，先将其划归为临时终端实例管辖
 */
bool CNetTerminalManager::ChannelArrived(DWORD nCID) {
    printf("terminalMAnager:channelArrived.1\n");
    bool bOK = false;
    //创建一个CNetChannel并将其加入临时终端所管辖的通道中
    //printf("[NCT::AutoTestDebug]::有新的通道连接到本终端\n");
    bOK = m_TempNetTerminal.CreateNetChannel(nCID);
    //printf("[NCT::AutoTestDebug]::通道创建结果:%d\n", bOK);
    return bOK;
}

/**
 * 当本机所连的某条TCP通道断开时，对其所属的终端做进一步处理
 */
bool CNetTerminalManager::ChannelLeaved(DWORD nCID) {
    bool bOK = false;
    bOK = TerminalTraverse(nCID);
    return bOK;
}

bool CNetTerminalManager::Recv_NetCommMessageNotify(DWORD nCID, bool bCreate) {
    bool bOK = false;
    if (bCreate) {
        printf("TerminalManager:Recv_NetCommMessageNotify.1\n");
        bOK = ChannelArrived(nCID);
    } else {
        printf("TerminalManager:Recv_NetCommMessageNotify.2\n");
        bOK = ChannelLeaved(nCID);
    }
    return bOK;
}

/**
 * 这个方式其实是注册到NetComm模块的关于接收到数据的回调函数的实体
 * isNeedSlit是指是否有拆粘包操作,即为TCP信息类,对于UDP数据类无此操作,只为false
 */
//mark 这个回调函数需要进一步研究优化
bool CNetTerminalManager::Recv_NetCommBufferNotify(DWORD dwCID, std::shared_ptr<BYTE> &pBuffer, DWORD nLength,
                                                   bool isNeedSlit) {
    bool bOK = false;
    //加锁？回调函数在底层被调用,当然会被多个线程同时调用此函数，是安全的吗？
    /*pthread_mutex_lock(&m_decode_lock);*/
    if (!isNeedSlit) {
        timeval tv;
        gettimeofday(&tv, nullptr);
        int64_t utc_stamp_get = (tv.tv_sec * 1000000 + tv.tv_usec) / 1000;
        std::shared_ptr<CNetBuffer> pNetBuffer = std::make_shared<CNetBuffer>(dwCID, pBuffer, nLength, utc_stamp_get);
        if (m_DataList.GetMyCount() <= NETCOMM_MAX_UNDEAL_DATA_NUMBER) {
            //未到最大未处理包数目
            DWORD dwDestination = 0;
            DWORD dwSourceTID = 0;
            DataCodec::GetRouterInfo(pBuffer, nLength, dwSourceTID, dwDestination);
            if (m_dwTID != dwDestination) {
                //转发
                SendData(dwDestination, pBuffer, nLength, true);
            } else {
                //存储
                m_DataList.AddTail(pNetBuffer);
            }
        } else {
            //达到最大未处理包数目，丢弃
            DWORD packet_need_remove = NETCOMM_MAX_UNDEAL_DATA_NUMBER / 4;
            while (packet_need_remove) {
                m_DataList.RemoveHead();
                packet_need_remove--;
            }
        }
        return true;
    }
    std::shared_ptr<TcpChannelDecode> TcpDecode_help = nullptr;
    if (m_tcpChannelDecodeHelp.find(dwCID) != m_tcpChannelDecodeHelp.end()) {
        TcpDecode_help = m_tcpChannelDecodeHelp.at(dwCID);
    } else {
        TcpDecode_help = std::make_shared<TcpChannelDecode>();
        TcpDecode_help->m_dwTempLength = 0;
        TcpDecode_help->m_pTempBuffer = nullptr;
        m_tcpChannelDecodeHelp.insert(std::pair<DWORD, std::shared_ptr<TcpChannelDecode>>(dwCID, TcpDecode_help));
        TcpDecode_help = m_tcpChannelDecodeHelp.at(dwCID);
    }
    std::shared_ptr<BYTE> &pLeftBuffer = TcpDecode_help->m_pTempBuffer;
    DWORD &dwLeftLength = TcpDecode_help->m_dwTempLength;
    //对上次没有处理完的数据做处理，主要是用于解决TCP粘包问题和拆包问题的
    bool bNeedMemoryClean = false;
    //如果上次还有数据没被处理，那么把这些数据拼接到pBuffer后面
    if (pLeftBuffer) {
        //先开辟一块儿字节数组，要大到能把原来没处理的数据和传入的待处理的数据都装进去
        std::shared_ptr<BYTE> pTempBuffer(new BYTE[dwLeftLength + nLength], releaseArrays<BYTE>);
        memcpy(pTempBuffer.get(), pLeftBuffer.get(), dwLeftLength);
        memcpy(pTempBuffer.get() + dwLeftLength, pBuffer.get(), nLength);
        //变量更新
        pBuffer = pTempBuffer;
        nLength += dwLeftLength;
        bNeedMemoryClean = true;//表示pBuffer在函数尾部需要释放掉
        //把原通道内剩余数据资源释放
        pLeftBuffer = nullptr;
    }
    if (pBuffer && pBuffer.get() && nLength > 0) {
        //如果确实有数据要处理
        //先判断是不是消息
        if (DataCodec::IsMessage(pBuffer, nLength)) {
            //因为目前只需要对TCP进行粘包拆包处理，而TCP是面向连接的，所以只需要知道TCP的通道号就好了
            DWORD dwCommand = CIMsg::DecodeCommandID(pBuffer.get(), nLength);
            bool bFindCommand = true;//用于判断Buffer是否是本模块可解析指令
            bool bNeedWaitForNextBuffer = false;//判断Buffer是否需要等待下一段数据到来
            bool bNeedSlit = false;//判断Buffer是否需要拆分
            DWORD nRealLength = 0;//记录Buffer消息的真实长度，即各个消息类中采用const static int形式规定的长度，本模块所接受的消息实际上都是定长的

            std::shared_ptr<CIMsg> pMsg = nullptr;

            switch (dwCommand) {
                case 0:
                    if (nLength < 5) {
                        bNeedWaitForNextBuffer = true;
                    }
                    break;
                case eCommandRemoteIDRequest: {
                    CIMsgRemoteIDRequest msg;
                    nRealLength = msg.GetLength();
                    if (nRealLength > nLength) {
                        bNeedWaitForNextBuffer = true;
                    } else {
                        if (nRealLength < nLength) {
                            bNeedSlit = true;
                        }
                        bOK = msg.Decode(pBuffer, nLength);
                        if (bOK) {
                            pMsg = msg.CopyFrom();
                        }
                    }
                }
                    break;
                case eCommandRemoteIDNotify: {
                    CIMsgRemoteIDNotify msg;
                    nRealLength = msg.GetLength();
                    if (nRealLength > nLength) {
                        bNeedWaitForNextBuffer = true;
                    } else {
                        if (nRealLength < nLength) {
                            bNeedSlit = true;
                        }
                        bOK = msg.Decode(pBuffer, nLength);
                        if (bOK) {
                            pMsg = msg.CopyFrom();
                        }
                    }
                }
                    break;
                case eCommandRemoteUDPPortRequest: {
                    CIMsgRemoteUDPPortRequest msg;
                    nRealLength = msg.GetLength();
                    if (nRealLength > nLength) {
                        bNeedWaitForNextBuffer = true;
                    } else {
                        if (nRealLength < nLength) {
                            bNeedSlit = true;
                        }
                        bOK = msg.Decode(pBuffer, nLength);
                        if (bOK) {
                            pMsg = msg.CopyFrom();
                        }
                    }
                }
                    break;
                case eCommandRemoteUDPPortNotify: {
                    CIMsgRemoteUDPPortNotify msg;
                    nRealLength = msg.GetLength();
                    if (nRealLength > nLength) {
                        bNeedWaitForNextBuffer = true;
                    } else {
                        if (nRealLength < nLength) {
                            bNeedSlit = true;
                        }
                        bOK = msg.Decode(pBuffer, nLength);
                        if (bOK) {
                            pMsg = msg.CopyFrom();
                        }
                    }
                }
                    break;
                case eCommandRouterInfoNotify: {
                    CIMsgRouterInfoNotify msg;
                    nRealLength = msg.GetLength();
                    if (nRealLength > nLength) {
                        bNeedWaitForNextBuffer = true;
                    } else {
                        if (nRealLength < nLength) {
                            bNeedSlit = true;
                        }
                        bOK = msg.Decode(pBuffer, nLength);
                        if (bOK) {
                            pMsg = msg.CopyFrom();
                        }
                    }
                }
                    break;
                case eCommandRouterInfoDeleteNotify: {
                    CIMsgRouterInfoDeleteNotify msg;
                    nRealLength = msg.GetLength();
                    if (nRealLength > nLength) {
                        bNeedWaitForNextBuffer = true;
                    } else {
                        if (nRealLength < nLength) {
                            bNeedSlit = true;
                        }
                        bOK = msg.Decode(pBuffer, nLength);
                        if (bOK) {
                            pMsg = msg.CopyFrom();
                        }
                    }
                }
                    break;
                case eCommandShortMessage: {
                    //printf("[NetCombTransferDebug]::RecvShortMessage !\n");
                    CIMsgShortMessage short_msg;
                    if (CIMsgShortMessage::DATA_HEADER_LENGTH <= nLength) {
                        nRealLength =
                                DataCodec::ReadData(pBuffer.get() + CIMsgShortMessage::DATA_HEADER_LENGTH_OFFSET) +
                                CIMsgShortMessage::DATA_HEADER_LENGTH;
                        if (nRealLength > nLength) {
                            bNeedWaitForNextBuffer = true;
                        } else {
                            if (nRealLength < nLength) {
                                bNeedSlit = true;
                            }
                            bOK = short_msg.Decode(pBuffer, nLength);
                            if (bOK) {
                                pMsg = short_msg.CopyFrom();
                            }
                        }
                    } else {
                        bNeedWaitForNextBuffer = true;
                    }
                }
                    break;
                default: {
                    bFindCommand = false;
                    DWORD dwDestinationTID = 0;
                    DWORD dwSourceTID = 0;
                    //mark 研究这个地方为什么是GetRouterBuffer
                    bOK = DataCodec::GetRouterInfo(pBuffer, nLength, dwSourceTID, dwDestinationTID);
                    if (dwSourceTID > MAX_TID_NUMBER) {
                        break;
                    }
                    timeval tv;
                    gettimeofday(&tv, nullptr);
                    int64_t utc_stamp_get = (tv.tv_sec * 1000000 + tv.tv_usec) / 1000;
                    std::shared_ptr<CNetBuffer> pNetBuffer = std::make_shared<CNetBuffer>(dwCID, pBuffer, nLength,
                                                                                          utc_stamp_get);
                    //printf("[NetCombTransferDebug]::MsgBufferList Add New Buffer !\n");
                    m_MsgBufferList.AddTail(pNetBuffer);
                    //printf("[NetCombTransfer] MsgBufferList Now has %d buffers!!!\n", m_MsgBufferList.GetMyCount());
                }
                    break;
            }
            if (bFindCommand) {
                if (bOK) {
                    if (pMsg) {
                        pMsg->m_dwDeviceID = dwCID;
                        m_MsgList.AddTail(pMsg);
                    } else {
                        bOK = false;
                    }
                    if (bNeedSlit) {
                        DWORD dwSplitiLength = nLength - nRealLength;
                        std::shared_ptr<BYTE> pNextBuffer(new BYTE[dwSplitiLength], releaseArrays<BYTE>);
                        memcpy(pNextBuffer.get(), pBuffer.get() + nRealLength, dwSplitiLength);
                        pBuffer = nullptr;
                        Recv_NetCommBufferNotify(dwCID, pNextBuffer, dwSplitiLength, true);
                    }
                } else if (bNeedWaitForNextBuffer) {
                    std::shared_ptr<BYTE> pTemp(new BYTE[nLength], releaseArrays<BYTE>);
                    pLeftBuffer = pTemp;
                    memcpy(pLeftBuffer.get(), pBuffer.get(), nLength);
                    dwLeftLength = nLength;
                }
            }
        } else {

        }
    }
    //mark,再研究一下这个标志位作业是否冗余
    if (bNeedMemoryClean) {
        pBuffer = nullptr;
    }
    return bOK;
}

bool CNetTerminalManager::Recv_NetQosMonitorMessageNotify(DWORD nCID, DWORD nNop, DWORD m_nQosSend, DWORD m_nQosRecv) {
    bool bOK = false;
    return bOK;
}


//将确定的CID移动到对应的TID中
bool CNetTerminalManager::MoveChannelToTerminal(const std::shared_ptr<CNetChannel> &pNetChannel) {
    bool bOK = false;
    //若已有终端类存在，则将该通道类直接加入相应终端类
    for (const auto &pTerminal: m_TerminalList.node_list) {
        if (pTerminal->m_dwTID == pNetChannel->m_dwTID) {
            //找到对应终端类并将该通道类加入
            pTerminal->AddNetChannel(pNetChannel);
            //将该通道类从临时终端类移除
            printf("[Debug]:MoveChannelToTerminal:旧终端的通道，%d通道已经认证，将其加入认证通道列表中\n", pNetChannel->m_dwCID);
            m_TempNetTerminal.RemoveNetChannel(pNetChannel);
            printf("[Debug]:MoveChannelToTerminal:临时终端内通道数:%zu\n", m_TempNetTerminal.NetChannelList.node_list.size());
            bOK = true;
            break;
        }
    }

    //如果未找到对应的已认证终端类，则创建新的终端类并将该通道加入
    if (!bOK) {
        //创建新的终端类并将该通道类加入
        shared_ptr<CNetTerminal> pNetTerminal = CreateNewTerminal(pNetChannel);
        m_TerminalList.AddTail(pNetTerminal);
        //将该通道类从临时终端类移除
        printf("[Debug]:MoveChannelToTerminal:新终端的通道,%d通道已经认证，将其加入认证通道列表中\n", pNetChannel->m_dwCID);
        m_TempNetTerminal.RemoveNetChannel(pNetChannel);
        printf("[Debug]:MoveChannelToTerminal:临时终端内通道数:%zu\n", m_TempNetTerminal.NetChannelList.node_list.size());
        printf("[Debug]:MoveChannelToTerminal:认证终端内通道数:%zu\n", m_TerminalList.node_list.size());
        //建立新的路由表信息
        DWORD dwTID = pNetTerminal->m_dwTID;

        printf("[DEBUG]:MoveChannelToTerminal: UpdateRouterInfo, TID:%d\n", m_dwTID);
        UpdateRouterInfo(pNetTerminal->m_dwTID, m_dwTID, 0, 0, 0);

        printf("[DEBUG]:MoveChannelToTerminal: notify up layers\n");
        //通知上层有新的远程终端接入
        for (int i = 0; i < GLOBAL_MAX_REGISTER_CALLBACK_FUN; ++i) {
            if (nullptr != m_pMessageCallBack[i]) {
                bOK = m_pMessageCallBack[i](dwTID, true);
                if (!bOK) {
                    return bOK;
                }
            }
        }
        /*for (auto &msgcallback: m_pMessageCallBack) {
            if (nullptr != msgcallback) {
                bOK = msgcallback(dwTID, true);
                if (!bOK) {
                    return bOK;
                }
            }
        }*/
        bOK = true;
    }
    //TODO:这里是修改的地方begin
    printf("[DEBUG]:MoveChannelToTerminal: terminalCallBack begin\n");
    terminalCallBack();
    printf("[DEBUG]:MoveChannelToTerminal: terminalCallBack end\n");
    //TODO:这里是修改的地方end
    return bOK;
}

//创建新的终端（已认证
std::shared_ptr<CNetTerminal> CNetTerminalManager::CreateNewTerminal(const std::shared_ptr<CNetChannel> &pNetChannel) {
    std::shared_ptr<CNetTerminal> pTerminal = std::make_shared<CNetTerminal>();
    pTerminal->m_dwTID = pNetChannel->m_dwTID;
    pTerminal->AddNetChannel(pNetChannel);
    return pTerminal;
}

//查找nTID指向的终端类
shared_ptr<CNetTerminal> CNetTerminalManager::TerminalFind(DWORD nTID) {
    m_TerminalList.Lock();
    for (shared_ptr<CNetTerminal> pTerminal: m_TerminalList.node_list) {
        if (nTID == pTerminal->m_dwTID) {
            m_TerminalList.UnLock();
            return pTerminal;
        }
    }
    m_TerminalList.UnLock();
    return nullptr;
}

/**
 * \brief 路由增添与更新函数
 * \param dwDestinationTID
 * \param dwSourceTID
 * \param m_nNop
 * \param m_nQosSend
 * \param m_nQoSRecv
 * \return 如果路由表确实有变动，那么返回true； 否则返回false
 */
bool CNetTerminalManager::UpdateRouterInfo(DWORD dwDestinationTID, DWORD dwSourceTID, DWORD m_nNop, DWORD m_nQosSend,
                                           DWORD m_nQosRecv) {
    //如果目的终端==本终端终端号，则为无效路由，丢弃
    if (m_dwTID == dwDestinationTID) {
        return false;
    }
    /*if (dwDestinationTID == dwSourceTID) {
        return false;
    }*/
    std::shared_ptr<CRouterInfo> NewRouterInfo =
            std::make_shared<CRouterInfo>(dwDestinationTID, dwSourceTID, m_nNop, m_nQosSend, m_nQosRecv);
    //查询对应的目的终端是否已经存在
    for (auto &TempRouterInfo: m_RouterInfoList.node_list) {
        if (TempRouterInfo->m_dwDestinationTID == dwDestinationTID) {
            if (TempRouterInfo->m_dwSourceTID != dwSourceTID) continue;
            if (*NewRouterInfo < *TempRouterInfo) {
                //如果新的信息比之前的信息小，那么就更新
                try {
#ifdef COMBTRANSFER_ROUTERINFO
#endif
                    TempRouterInfo = NewRouterInfo;
                    if (0 == ROUTE_CALLBACK_NUMBER) {
                        return true;
                    }
                    CallAllRouteCallBacks(dwSourceTID, dwDestinationTID, m_nNop, m_nQosSend, m_nQosRecv, true);
                    return true;
                }
                catch (...) {
                    return false;
                }
            } else {
                /*if (TempRouterInfo->m_dwSourceTID != dwSourceTID) {
                    continue;
                } else {
                    return false;
                }*/
                return false;
            }
        }
    }
//对应的到该节点的路由信息并没有
//建立新的路由节点
//将新的路由节点加入路由表中
    if (NewRouterInfo) {
        try {
            m_RouterInfoList.AddTail(NewRouterInfo);
#ifdef COMBTRANSFER_ROUTERINFO
#endif
//调用回调函数通知上层
            if (0 == ROUTE_CALLBACK_NUMBER) {
                return true;
            }
            CallAllRouteCallBacks(dwSourceTID, dwDestinationTID, m_nNop, m_nQosSend, m_nQosRecv,
                                  true);
            return true;
        } catch (...) {
            return false;
        }
    } else {
        return false;
    }
}

/**
 *
 */
bool CNetTerminalManager::SendRouterInfo(DWORD dwDestinationTID, DWORD dwCID) {
    bool bOK = false;
    for (const std::shared_ptr<CRouterInfo> &TempRouterInfo: m_RouterInfoList.node_list) {
        //跳过dwDestinationTID对应的路由信息
        if (dwDestinationTID == TempRouterInfo->m_dwDestinationTID) {
            continue;
        }
        //编码路由增量信息
        std::shared_ptr<CIMsgRouterInfoNotify> msg = std::make_shared<CIMsgRouterInfoNotify>();
        msg->m_dwDestinationTID = TempRouterInfo->m_dwDestinationTID;
        msg->m_dwSourceTID = m_dwTID;
        msg->m_nNop = TempRouterInfo->m_nNop + 1;
        msg->m_nQosSend = TempRouterInfo->m_nQoSSend;
        msg->m_nQosRecv = TempRouterInfo->m_nQoSRecv;

        bOK = SendCommand(msg, dwCID);
        if (!bOK) {
            return false;
        } else {
#ifdef COMBTRANSFER_ROUTERINFO
#endif
        }
    }
    return true;
}


/**
 * 这个函数的应用场景：从终端dwExtid传来从dwExtid到dwDestinationTID的路由信息，然后如果这条记录引起更新的话就调用此函数
 * 这也解释了为什么要忽略dwExtid，因为此条路由消息就是从那传来的，再发过去没有意义。
 */
/*这个函数用于通知邻居自己新获得路由条目,具体解释如上*/
bool CNetTerminalManager::SendRouterInfoToAll(DWORD dwExTID, DWORD dwDestinationTID, DWORD nNop, DWORD m_nQosSend,
                                              DWORD m_nQosRecv) {
    bool bOK = false;

    std::shared_ptr<CIMsgRouterInfoNotify> pMsg = std::make_shared<CIMsgRouterInfoNotify>();
    pMsg->m_dwDestinationTID = dwDestinationTID;
    pMsg->m_dwSourceTID = m_dwTID;
    pMsg->m_nNop = nNop + 1;
    //向本终端的终端列表中的所有终端发送此路由信息
    //先遍历持有的终端列表中所有终端
    for (const std::shared_ptr<CNetTerminal> &pTerminal: m_TerminalList.node_list) {
        //跳过路由消息来源的终端
        if (dwExTID == pTerminal->m_dwTID) {
            continue;
        }
        //发送此路由信息
        std::shared_ptr<CNetChannel> pNetChannel = pTerminal->ChannelSelect();
        if (pNetChannel) {
            DWORD dwChannelNum = pNetChannel->m_dwCID;
            bOK = SendCommand(pMsg, dwChannelNum);
        } else {
            return false;
        }
        if (!bOK) {
            return false;
        }
#ifdef COMBTRANSFER_ROUTERINFO
#endif
    }
    return true;
}

//
bool CNetTerminalManager::SendRouterInfoToAll(DWORD dwExTID, DWORD dwDestinationTID) {
    bool bOK = false;

    std::shared_ptr<CIMsgRouterInfoDeleteNotify> pMsg = std::make_shared<CIMsgRouterInfoDeleteNotify>();
    pMsg->m_dwDestinationTID = dwDestinationTID;
    pMsg->m_dwSourceTID = m_dwTID;
    //遍历终端实例，发送路由删除消息
    for (const auto &pTerminal: m_TerminalList.node_list) {
        if (dwExTID == pTerminal->m_dwTID) {
            //跳过终端号为dwExtid的终端
            continue;
        }
        std::shared_ptr<CNetChannel> pNetChannel = pTerminal->ChannelSelect();
        if (pNetChannel) {
            DWORD dwChannelNum = pNetChannel->m_dwCID;
            bOK = SendCommand(pMsg, dwChannelNum);
        } else {

            return false;
        }
        if (!bOK) {
            return false;
        }
#ifdef COMBTRANSFER_ROUTERINFO
#endif
    }
    return true;
}

bool CNetTerminalManager::DeleteRouterInfo(DWORD dwDestinationTID, DWORD dwSourceTID) {
    NETCombTransferBufferList<CRouterInfo> temp_will_replace;       //即将代替老的路由表的新路由表
    NETCombTransferBufferList<CRouterInfo> temp_router_has_been_remove;     //存储被删掉的路由
    m_RouterInfoList.Lock();
    for (const auto &pRouterInfo: m_RouterInfoList.node_list) {
        if (dwDestinationTID == pRouterInfo->m_dwDestinationTID) {
            try {
                temp_router_has_been_remove.AddTail(pRouterInfo);
            } catch (...) {
                m_RouterInfoList.UnLock();
                return false;
            }
        } else {
            temp_will_replace.AddTail(pRouterInfo);
        }
    }

    //通过回调函数通知上层路由发生了变化
    for (const auto &pRouterInfo: temp_router_has_been_remove.node_list) {
#ifdef COMBTRANSFER_ROUTERINFO
#endif
        if (0 == ROUTE_CALLBACK_NUMBER) {

        } else {
            //printf("[TempTest]::DeleteRouterInfo called\n");
            /*if (0 != pRouterInfo->m_nNop) {
                //函数用于通知上层dwSource->dwDestination的链路断开了,但是如果跳数为0(即直接和本地相连的终端)会通过termonoff收到通知
                CallAllRouteCallBacks(dwSourceTID, dwDestinationTID, -1, -1, -1, false);
            }*/
            CallAllRouteCallBacks(dwSourceTID, dwDestinationTID, -1, -1, -1, false);
        }
    }
    temp_router_has_been_remove.node_list.clear();
    /*m_RouterInfoList = temp_will_replace;*/
    m_RouterInfoList.UnLock();
    m_RouterInfoList = temp_will_replace;
    return true;
}

DWORD CNetTerminalManager::FindNextHop(DWORD dwDestinationTID) {
    for (const auto &pRouterInfo: m_RouterInfoList.node_list) {
        if (!pRouterInfo) {
            return -1;
        }
        if (dwDestinationTID == pRouterInfo->m_dwDestinationTID) {
            //说明路由是存在的
            //如果是本机直连的终端
            if (0 == pRouterInfo->m_nNop) {
                return dwDestinationTID;
            } else {
                //否则返回远端TID,即下一条对应的转发终端TID
                return pRouterInfo->m_dwSourceTID;
            }
        }
    }
    //如果未找到对应的路由条目，那么返回错误码-1
    return -1;
}

//发送路由更新命令以移除由离开的终端传来的路由信息条目信息
bool CNetTerminalManager::SendRouterInfoToAllAndUpdate(DWORD dwSourceTID) {
    bool bOK = true;
    m_RouterInfoList.Lock();
    std::deque<std::shared_ptr<CRouterInfo>> TempRouterInfoList;
    for (auto &pRouterInfo: m_RouterInfoList.node_list) {
        if (dwSourceTID == pRouterInfo->m_dwSourceTID) {

            try {
                bOK = SendRouterInfoToAll(dwSourceTID, pRouterInfo->m_dwDestinationTID);

            } catch (...) {
                m_RouterInfoList.UnLock();
                return false;
            }
            if (bOK) {
                try {
                    if (0 == ROUTE_CALLBACK_NUMBER) {
                    }
                    //回调函数通知上层
//                    printf("[TempTest]::SendRouterInfoToAllAndUpdate called\n");
                    CallAllRouteCallBacks(dwSourceTID, pRouterInfo->m_dwDestinationTID, -1, -1, -1, false);
                    //通知BigDataTransfer模块关闭向router_info->m_dwDestinationTID开启的数据发送任务
                    if (nullptr != m_pMessageCallBack[0]) {
                        bOK = m_pMessageCallBack[0](pRouterInfo->m_dwDestinationTID, false);
                        //log
                    }
                    continue;
                } catch (...) {
                    //这是为了避免因异常导致的死锁
                    m_RouterInfoList.UnLock();
                    return false;
                }
            } else {
                //路由更新命令发送失败
                m_RouterInfoList.UnLock();
                return false;
            }
        } else {
            TempRouterInfoList.push_back(pRouterInfo);
        }
    }
    m_RouterInfoList.node_list = TempRouterInfoList;
    m_RouterInfoList.UnLock();
    return true;
}


//TODO:这里是修改
/**
 * 该函数用于向上层通知连接的终端列表
 * @param p_Net_MessageCallback
 * @return
 */
bool CNetTerminalManager::DoRegister_TerminalCallBack(pNetMessageCallback p_Net_MessageCallback){
    m_pTerminalMessageCallBack=p_Net_MessageCallback;
    return true;
}

/**
 * \brief 这个函数用于注册一对消息和数据回调函数，这个操作不具有原子性，只要都注册成功即可，注意把回调函数为nullptr的情况视为注册成功
 * \param pMsgCallback
 * \param pDataCallback
 * \return 两个回调函数都注册成功则返回true，否则返回false
 */
bool CNetTerminalManager::DoRegister_NetCombTransferCallBack(pNetCombTransferMessageCallback pMsgCallBack,
                                                             pNetCombTransferBufferCallback pDataCallBack) {
    bool bMsgCallBackRegister = false;
    int nMsgCallBackRegister = 0;

    bool bDataCallBackRegister = false;
    int nDataCallBackRegister = 0;

    if (nullptr != pMsgCallBack || nullptr != pDataCallBack) {
        //先注册消息回调函数
        for (int i = 0; i < GLOBAL_MAX_REGISTER_CALLBACK_FUN; ++i) {
            if (nullptr == m_pMessageCallBack[i]) {
                m_pMessageCallBack[i] = pMsgCallBack;
                nMsgCallBackRegister = i;
                bMsgCallBackRegister = true;
                break;
            }
        }

        //再注册数据回调函数
        for (int i = 1; i < GLOBAL_MAX_REGISTER_CALLBACK_FUN; ++i) {
            if (nullptr == m_pDataCallBack[i]) {
                m_pDataCallBack[i] = pDataCallBack;
                nDataCallBackRegister = i;
                bDataCallBackRegister = true;
                break;
            }
        }
    } else {
        return true;
    }
    return true;
}

//路由回调函数注册
bool CNetTerminalManager::DoRegister_NetCombTransferRouterCallBack(
        pNetCombTransferRouterNotify p_comb_transfer_router_notify) {
    if (nullptr != p_comb_transfer_router_notify && ROUTE_CALLBACK_NUMBER < ROUTE_CALLBACK_NUMBER_MAX) {
        m_pRouteCallBack[ROUTE_CALLBACK_NUMBER] = p_comb_transfer_router_notify;
        ROUTE_CALLBACK_NUMBER++;
        return true;
    } else {
        return false;
    }
}

bool CNetTerminalManager::TmpTerminalStateTraverse() {
    bool ret = false;

    //驱动通道认证过程
    ret = m_TempNetTerminal.ChannelTraverse();

    if (!ret) {
        //log
    }
    return ret;
}

bool CNetTerminalManager::IsRouterOn(DWORD dwDestinationTID) {
    for (const auto &pRouterInfo: m_RouterInfoList.node_list) {
        if (dwDestinationTID == pRouterInfo->m_dwDestinationTID) {
            //查询到存在到dwDestinationTID的路由条目
            return true;
        }
    }

    //没有查询到dwDestinationTID的路由条目
    return false;
}


//20230324
bool CNetTerminalManager::TcpChannelActiveCloseByNetComm(DWORD nCID, bool bCreate) {
    //CID是唯一的，只需在所有的pTerminal中找到通道号是CID的channel即可
    bool bOK = false;
    if (!bCreate) {
        bOK = TerminalTraverse(nCID);
    }
    return bOK;
}

//将终端列表信息传递给terminal列表中，并调用回调函数向上层返回终端列表信息
void CNetTerminalManager::terminalCallBack(){
    printf("[DEBUG]:terminalCallBack:terminalManager中执行回调。。。\n");
    //printf("[DEBUG]:terminalCallBack:m_TerminalList size: %s\n", m_TerminalList.node_list.size());

    std::vector<Terminal> terminalList;
    Terminal temp;
    bool flag= false;
    printf("[DEBUG]:terminalCallBack:遍历terminalList start\n");
    for(const auto terminalTemp:m_TerminalList.node_list){
        printf("[DEBUG]:terminalCallBack:遍历terminalList，至少存在一个终端。\n");
        flag= true;
        temp.TID=terminalTemp->m_dwTID;
        temp.channelNum=terminalTemp->NetChannelList.GetMyCount();
        temp.localPort=terminalTemp->NetChannelList.GetMyHead()->m_nLocalUDPPort;
        temp.localAddress=terminalTemp->NetChannelList.GetMyHead()->m_szLocalAddress;
        temp.remotePort=terminalTemp->NetChannelList.GetMyHead()->m_nRemoteUDPPort;
        temp.remoteAddress=terminalTemp->NetChannelList.GetMyHead()->m_szRemoteAddress;
        temp.state=1;  //表示已连接
        terminalList.push_back(temp);
    }
    printf("[DEBUG]:terminalCallBack:遍历terminalList end\n");
    /*if(flag){//至少有一个终端
        //将新的终端列表传递给上层
        m_pTerminalMessageCallBack(terminalList);
    }else{
        m_pTerminalMessageCallBack(nullptr);
    }*/
    if(m_pTerminalMessageCallBack==nullptr) {
        printf("[DEBUG]:terminalCallBack:m_pTerminalMessageCallBack is nullptr!!!\n");
    }
    else{
        printf("[DEBUG]:terminalCallBack:m_pTerminalMessageCallBack is not nullptr.\n");
        printf("[DEBUG]:terminalCallBack:m_pTerminalMessageCallBack start run\n");
        m_pTerminalMessageCallBack(terminalList);
        printf("[DEBUG]:terminalCallBack:m_pTerminalMessageCallBack end run\n");
    }
}
