  //
// Created by wangbingqi on 22-9-30.
//

#ifndef EPOLLCOMM_CNETCOMMEPOLL_H
#define EPOLLCOMM_CNETCOMMEPOLL_H

#include "CTCP.h"
#include "CUDP.h"
#include "CBCast.h"
#include "ChannelManager.h"
#include "CNetChannelList.h"


class CNetCommEpoll {
public:
    CNetCommEpoll();

    ~CNetCommEpoll();

public:
    //PDATACALLBACK m_pReceiveDataCallback;
    PMESSAGECALLBACK m_pReceiveMessageCallback = nullptr;
    PMESSAGECALLBACK m_pReceiveBCastMessageCallback = nullptr;    //这个回调函数主要用于对NeighborSearch的回调
    PMESSAGECALLBACK m_pChannelOnOffCallbackForThirdParty = nullptr; //这个回调函数主要用于对ThirdPartyTransfer的回调，通知其第三方接口上下线
    PMESSAGECALLBACK m_pChannelOnOffCallbackForCombTransfer = nullptr; //20230324,添加：
    PDATACALLBACK m_pReceiveDataCallback[GLOBAL_MAX_REGISTER_CALLBACK_FUN];
    PDATACALLBACK m_pReceiveDataForThirdPartyTransfer = nullptr;

    //这个函数设置了两种消息传递途径，一种是通过句柄和句柄对应消息值传递信息，一种是通过回调函数传递信息
    void SetInterfaceParam(PMESSAGECALLBACK pMsgCallback, PMESSAGECALLBACK pMsgCallback2, PDATACALLBACK pDataCallback);

    void pDataCallBackRegister(PDATACALLBACK *pDataCallBackFunc);

    friend DWORD /*WINAPI*/ ThreadProcDetectTCPSelfKeepAlive(void *g_pIOCPSocketDlg);

    //线程函数，通过回调函数来反馈TCP链路信息（通过系统自己的心跳机制来判断哪）
    friend /*EXPORT_NETCOMM_API*/ bool WriteTcpKeepAlivePacketChannel(DWORD Channel);

    //这个函数设置了对QoS模块的回调函数
    void DoRegister_BCastMessageCallback(PMESSAGECALLBACK pMessageCallback) {
        m_pReceiveBCastMessageCallback = pMessageCallback;
    }

    void Register_NetCommChannelOnOffCallbackForThirdParty(PMESSAGECALLBACK pMessageCallback) {
        m_pChannelOnOffCallbackForThirdParty = pMessageCallback;
    }

    void Register_NetCommChannelOnOffCallbackForCombTransfer(PMESSAGECALLBACK pMessageCallback) {
        m_pChannelOnOffCallbackForCombTransfer = pMessageCallback;
    }

public:
    //栈上数据，是否转移到堆区
    std::unique_ptr<CNetChannelist<CTCP>> mTcpList;
    std::unique_ptr<CNetChannelist<CUDP>> mUdpList;
    std::unique_ptr<CNetChannelist<CBCast>> mBcastList;
    /*new channelmanager*/
    std::unique_ptr<ChannelManager> gChannelManager;

    //取消在netcommmanager中管理eventlist，eventlist设置成由每个线程单独持有
    int TcpEpollFd;
    //epoll_event *TcpEvents;
    int UdpEpollFd;
    //epoll_event *UdpEvents;
    int BCastEpollFd;
    //epoll_event *BCastEvents;
    bool m_bInit = false;
public:
    //考虑函数是否可重入
    std::shared_ptr<CTCP> CreateTCP();

    std::shared_ptr<CUDP> CreateUDP();

    std::shared_ptr<CUDP> CreateBothWayUDP();

    std::shared_ptr<CBCast> CreateBCast();

    std::shared_ptr<CTCP> GetTCPBySock(int sockfd);

    std::shared_ptr<CUDP> GetUDPBySock(int sockfd);

    std::shared_ptr<CTCP> GetTCP(DWORD Channel) const;

    std::shared_ptr<CUDP> GetUDP(DWORD Channel) const;

    std::shared_ptr<CBCast> GetBCast(DWORD Channel) const;

    //Read***Buffer()在项目中几乎没有用到，暂不实现
    std::shared_ptr<BYTE> ReadTcpBuffer();

    std::shared_ptr<BYTE> ReadUdpBuffer();

    void DeleteTCP(std::shared_ptr<CTCP> TempTcp) const;

    void DeleteUDP(const std::shared_ptr<CUDP> &TempUdp) const;

    void DeleteBCast(const std::shared_ptr<CBCast> &TempBCast) const;

    bool WriteTcpChannel(DWORD Channel, const std::shared_ptr<BYTE> &sendbuf, DWORD length) const;

    bool WriteUdpChannel(DWORD Channel, const std::shared_ptr<BYTE> &sendbuf, DWORD length);

    bool WriteBCastChannel(DWORD Channel, std::shared_ptr<BYTE> sendbuf, DWORD length) const;

    /*bool CloseTcpChannel(DWORD dwChannel);*/

    void SetTcpServerChannelRemoteAddress();

    void SetTcpClientChannelRemoteAddress();

    void SetUdpServerChannelRemoteAddress(DWORD CID, const std::string &remote_address, DWORD remotePort) const;

    void SetUdpClientChannelRemoteAddress();

    bool NewCloseTcpChannel(DWORD dwChannel);

    bool CloseUdpChannel(DWORD dwChannel);
/*旧的通道管理机制*/
public:
    bool ReloadTCPChannel(DWORD Channel); //如果有TCP连接的通道号与Channel相同那么就删除这些通道，同时将通道号比其大的通道的通道号都减一，以实现TCP通道重排的效果
    std::shared_ptr<CTCP> CopyTcp(CTCP *TempTcp);          //目前用于将TCP客户端连接加入TCP链路集合
    std::shared_ptr<CTCP> NewCopyTcp(std::shared_ptr<CTCP> TcpServer, DWORD dwChannel);

    bool AddTCPChannel(const std::shared_ptr<CTCP> &TempTcp);
/*新的通道管理机制*/
public:


public:
    void CloseAllSocket() const;

    void CloseAllSocket(const std::string closeIpAddrress);

    void CloseAllTcpSocket(const std::string close_tcp_IpAddrress);

    void CloseAllBCastSocket(const std::string close_bcast_IpAddrress);

public:
    void HandleTcpEvents(epoll_event *EventList, int EventNum);

    void HandleUdpEvents(epoll_event *udpEvents, int EventNum);

    void HandleBCastEvents(epoll_event *bcastEvents, int EventNum) const;

public:
    bool DoRegister_NetCommCallBack(PDATACALLBACK pDataCallback);

    void DoRegister_NetCommThirdPartyTransferDataCallback(PDATACALLBACK pDataCallback);

private:

};


#endif //EPOLLCOMM_CNETCOMMEPOLL_H
