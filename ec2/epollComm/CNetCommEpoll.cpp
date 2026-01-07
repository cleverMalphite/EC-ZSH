//
// Created by wangbingqi on 22-9-30.
//
#include "CNetCommEpoll.h"

#include "infoHub/rateStatisticTable.h"

#include <utility>


CNetCommEpoll::CNetCommEpoll() {
//    printf("[NetcommManager created]\n");
    TcpEpollFd = epoll_create(TcpMaxChannelNum);
    UdpEpollFd = epoll_create(UdpMaxChannelNum);
    BCastEpollFd = epoll_create(BcastMaxChannelNum);
    for (int i = 0; i < GLOBAL_MAX_REGISTER_CALLBACK_FUN; i++) {
        m_pReceiveDataCallback[i] = nullptr;
    }
    //考虑使用智能指针
    gChannelManager = std::make_unique<ChannelManager>();
    mTcpList = std::make_unique<CNetChannelist<CTCP>>();
    mUdpList = std::make_unique<CNetChannelist<CUDP>>();
    mBcastList = std::make_unique<CNetChannelist<CBCast>>();
    m_bInit = true;
}

CNetCommEpoll::~CNetCommEpoll() {
    m_bInit = false;
    close(TcpEpollFd);
    close(UdpEpollFd);
    close(BCastEpollFd);
}

std::shared_ptr<CTCP> CNetCommEpoll::CreateTCP() {
    std::shared_ptr<CTCP> TempTcp = std::make_shared<CTCP>(TcpEpollFd, m_pReceiveDataCallback);
    if (TempTcp) {
        mTcpList->AddTail(TempTcp);
    }
    return TempTcp;
}

std::shared_ptr<CUDP> CNetCommEpoll::CreateUDP() {
    std::shared_ptr<CUDP> TempUdp = std::make_shared<CUDP>(UdpEpollFd, m_pReceiveDataCallback, true, false);
    if (TempUdp) {
        mUdpList->AddTail(TempUdp);
        //printf("CHANNELINFO:\nChannelNum{ %d }, UdpEpollFd{ %d }\n", TempUdp->m_dwChannel, UdpEpollFd);
    }
    return TempUdp;
}

//need test
std::shared_ptr<CUDP> CNetCommEpoll::CreateBothWayUDP() {
    std::shared_ptr<CUDP> TempUdpChannel = std::make_shared<CUDP>(UdpEpollFd, m_pReceiveDataCallback, true, true);
    if (TempUdpChannel) {
        mUdpList->AddTail(TempUdpChannel);
    }
    return TempUdpChannel;
}

//todo:needTest:20221026
//不安全，没有排查
/*bool CNetCommEpoll::CloseTcpChannel(DWORD dwChannel) {
    //更新NetCommManager的mTcpList队列
    std::shared_ptr<CTCP> TempChannel = mTcpList->GetByChannelNum(dwChannel);
    if (TempChannel) {
        if (!TempChannel->tcp_close()) {
            //printf("TCPCHANNEL CLOSE FAILED\n");
        }
        mTcpList->RemoveAt(TempChannel);

        if (nullptr != m_pChannelOnOffCallbackForThirdParty) {
            m_pChannelOnOffCallbackForThirdParty(dwChannel, false);
        }
    }

    //todo:对删除了通道对TcpServer的ChannelList进行重排，考虑使用ResortTcpChannel()函数
    //更新TcpServer中的ChannelList，只有Server才管理着一个ChannelList
    for (auto &TcpChannel: mTcpList->NetChannel) {
        if (TcpChannel->GetChannel() >= dwChannel) {
            TcpChannel->m_dwChannel--;
        }
        if (TcpChannel->tcp_IsServer()) { //找到Server
            //printf("Find TcpServer, Start update ChannelList\n");
            for (int i = 0; i < TcpChannel->mChannelList.Size(); i++) {
                std::shared_ptr<NODE> tempchannel = TcpChannel->mChannelList.GetAt(i);
                if (tempchannel->ChannelNumber == dwChannel) {
                    TcpChannel->mChannelList.RemoveAt(i);
                    //printf("Delete in Server ChannelList,ChannelNumber:{%ld}\n", dwChannel);
                    TcpChannel->m_dwChannelCurrentNumber--;
                    *//*//printf("m_dwChannelCurrentNumber/m_dwChannelNumber is (%ld/%ld)\n",
                           TcpChannel->m_dwChannelCurrentNumber, TcpChannel->m_dwChannelNumber);*//*
                    //成功删除无效node，开始更新node列表内容
                    for (int j = 0; j < TcpChannel->mChannelList.Size(); j++) {
                        std::shared_ptr<NODE> tempchannel1 = TcpChannel->mChannelList.GetAt(j);
                        if (tempchannel1->ChannelNumber >= dwChannel) {
                            tempchannel1->ChannelNumber--;
                        }
                    }
                    break;
                }
            }
        }
    }
    return true;
}*/
//TODO:SAFETEST
bool CNetCommEpoll::NewCloseTcpChannel(DWORD dwChannel) {
    //printf("[AutoTestDebug]::开始关闭TCP通道:%d\n", dwChannel);
    if (dwChannel > (MAXCHANNELNUM - 1))return false;//此dwchannel超出了数组的上界
    gChannelManager->lock();
    short ChannelStat = gChannelManager->GetChannelStat(dwChannel);
    std::shared_ptr<CTCP> TempChannel = nullptr;
    //更新channelstat
    //mtcplist删除通道，并通知
    //server更新channellist
    switch (ChannelStat) {
        //关闭服务器监听通道，通常意味着服务器关闭，则关闭此TcpServer的所有通道。ListenChannel的值在其中总是最小的。
        //范围中的状态只有ServerChannel和ServerTransChannel
        case ServerListenChannel://不需要更新channellist
            //printf("ServerListenChannel close\n");
            while (gChannelManager->GetChannelStat(dwChannel) != ClientChannel
                   && gChannelManager->GetChannelStat(dwChannel) != UnKnowChannel) {
                if (gChannelManager->GetChannelStat(dwChannel) == ServerChannel) {
                    gChannelManager->SetChannelStat(dwChannel, UnKnowChannel);
                    dwChannel++;
                    continue;
                }
                gChannelManager->SetChannelStat(dwChannel, UnKnowChannel);
                gChannelManager->unlock();
                TempChannel = mTcpList->GetByChannelNum(dwChannel);
                if (!TempChannel || !TempChannel.get()) {
                    dwChannel++;
                    if (dwChannel > (MAXCHANNELNUM - 1))break;
                    continue;
                }
                if (!TempChannel->tcp_close()) {
                    //printf("Server Close Failed\n");
                }
                DeleteTCP(TempChannel);
                if (nullptr != m_pChannelOnOffCallbackForThirdParty) {
                    m_pChannelOnOffCallbackForThirdParty(dwChannel, false);
                }
                dwChannel++;
                if (dwChannel > (MAXCHANNELNUM - 1))break;//防止索引越界
            }
            break;
        case ServerTransChannel://如果是使用中的ServerChannel，关闭即变为未使用状态。此时需要更新channellist
            //printf("ServerTransChannel close\n");
            gChannelManager->SetChannelStat(dwChannel, ServerChannel);
            TempChannel = mTcpList->GetByChannelNum(dwChannel);//删除此transServer
            if (!TempChannel || !TempChannel.get())return true;//保证安全
            if (!TempChannel->tcp_close()) {
                //printf("ServerTrans Close Failed\n");
            }
            DeleteTCP(TempChannel);//关闭并删除此ServerTransChannel
            for (DWORD i = dwChannel; i >= (dwChannel - TcpMaxChannelNum); i--) {
                if (gChannelManager->GetChannelStat(i) == ServerListenChannel) {
                    dwChannel = i;
                    break;
                }
            }//找到Server的channel号
            gChannelManager->unlock();
            TempChannel = mTcpList->GetByChannelNum(dwChannel);//找到了Serverchannel
            if (!TempChannel || !TempChannel.get())return true;//保证安全
            /*if (!TempChannel->tcp_close()) {
                //printf("Server Close Failed\n");
            }*/
            if (!TempChannel->tcp_IsServer())return false;
            TempChannel->mChannelList.Lock();
            for (int i = 0; i < TempChannel->mChannelList.Size(); i++) {//目前对于一个Server内的MCPLIST还没有加锁，在操作时存在风险。
                std::shared_ptr<NODE> tempchannel = TempChannel->mChannelList.GetAt(i);
                if (tempchannel->ChannelNumber == dwChannel) {
                    TempChannel->mChannelList.RemoveAt(i);
                    TempChannel->m_dwChannelCurrentNumber--;
                    break;
                }
            }
            TempChannel->mChannelList.UnLock();
            /*if (nullptr != m_pChannelOnOffCallbackForThirdParty) {
                m_pChannelOnOffCallbackForThirdParty(dwChannel, false);
            }*/
            break;
        case ServerChannel:// 一般不存在这种情况
            //printf("This channel belongs to a server, but is not used, delete failed\n");
            gChannelManager->unlock();
            break;
        default:
            //clientChannel直接关闭
            //printf("default close\n");
            gChannelManager->SetChannelStat(dwChannel, UnKnowChannel);
            gChannelManager->unlock();
            TempChannel = mTcpList->GetByChannelNum(dwChannel);
            if (!TempChannel || !TempChannel.get())return true;
            if (!TempChannel->tcp_close()) {
                //printf("Channel Close Failed\n");
            }
            DeleteTCP(TempChannel);//此时无需判断Tempchannel是否有效
            /*if (nullptr != m_pChannelOnOffCallbackForThirdParty) {
                m_pChannelOnOffCallbackForThirdParty(dwChannel, false);
            }*/
            if (nullptr != m_pChannelOnOffCallbackForCombTransfer) {
                m_pChannelOnOffCallbackForCombTransfer(dwChannel, false);
            }
            break;
    }
    return true;
}

bool CNetCommEpoll::CloseUdpChannel(DWORD dwChannel) {
    std::shared_ptr<CUDP> TempUdp = mUdpList->GetByChannelNum(dwChannel);
    if (dwChannel <= MAXCHANNELNUM) {
        gChannelManager->lock();
        gChannelManager->SetChannelStat(dwChannel, UnKnowChannel);
        gChannelManager->unlock();
    } else {
        return false;
    }
    if (TempUdp) {
        if (TempUdp->udp_close()) {
            return false;
        }
        DeleteUDP(TempUdp);
        return true;
    } else {
        return true;
    }
}

// TODO houlc-why : why need this feature ? 
//可重入，尽管copyTcp的操作不会是并行的
//CTCP* 应升级成shared_ptr防止多线程出错。(已完成)
std::shared_ptr<CTCP> CNetCommEpoll::NewCopyTcp(std::shared_ptr<CTCP> TcpServer, DWORD dwChannel) {
    if (!TcpServer || !TcpServer.get()) return nullptr;
    std::shared_ptr<CTCP> TempTransTcp = CreateTCP();
    if (TempTransTcp) {
        if (TempTransTcp->tcp_newCopy(TcpServer, dwChannel)) {
            //TempClientTcp->m_dwChannelCurrentNumber = (mTcpList.GetNetnum() - 1);
            return TempTransTcp;
        }
    }
    return nullptr;
}

//安全
bool CNetCommEpoll::AddTCPChannel(const std::shared_ptr<CTCP> &TempTcp) {
    if (TempTcp) {
        mTcpList->AddTail(TempTcp);
        return true;
    }
    return false;
}

//todo:getTCPbysockfd
std::shared_ptr<CTCP> CNetCommEpoll::GetTCPBySock(int sockfd) {
    return nullptr;
}

std::shared_ptr<CTCP> CNetCommEpoll::GetTCP(DWORD Channel) const {
    //内部已上锁，安全
    std::shared_ptr<CTCP> TempTcp = mTcpList->GetByChannelNum(Channel);
    return TempTcp;
}

//安全
void CNetCommEpoll::DeleteTCP(std::shared_ptr<CTCP> TempTcp) const {
    if (TempTcp) {
        mTcpList->RemoveAt(TempTcp);
    }
}

//todo:getUDPbysocket
std::shared_ptr<CUDP> CNetCommEpoll::GetUDPBySock(int sockfd) {
    return nullptr;
}

//安全
std::shared_ptr<CUDP> CNetCommEpoll::GetUDP(DWORD Channel) const {
    std::shared_ptr<CUDP> TempUdp = mUdpList->GetByChannelNum(Channel);
    return TempUdp;
}

void CNetCommEpoll::SetUdpServerChannelRemoteAddress(DWORD CID, const std::string &remote_address, DWORD remotePort) const {
    std::shared_ptr<CUDP> TempUdp = mUdpList->GetByChannelNum(CID);
    if (TempUdp && TempUdp.get()) {
        TempUdp->SetUdpServerChannelRemoteAddress(remote_address, remotePort);
    }
}


void CNetCommEpoll::DeleteUDP(const std::shared_ptr<CUDP> &TempUdp) const {
    if (TempUdp) {
        mUdpList->RemoveAt(TempUdp);
    }
}

//安全，进一步看tcp_write
bool CNetCommEpoll::WriteTcpChannel(DWORD Channel, const std::shared_ptr<BYTE> &sendbuf, DWORD length) const {
    std::shared_ptr<CTCP> TempTcp = mTcpList->GetByChannelNum(Channel);
    if (TempTcp) {
        //printf("normal send\n");
        bool ret = TempTcp->tcp_write(sendbuf, length);
        return ret;
    }
    return false;
}

//todo:handleTcpEvents param:EventList is not useful;
void CNetCommEpoll::HandleTcpEvents(epoll_event *EventList, int EventNum) {
    //printf("mTcpList:useCount is %ld\n", mTcpList->NetChannel[0].use_count());
    if (!m_bInit) {
        return;
    }
    int i = 0;
    int sockfd = 0;
    uint32_t event = 0;
    long HandleRet = 0;
    DWORD dwChannel = 0;
    std::shared_ptr<CTCP> TempTcp = nullptr;
    std::shared_ptr<CTCP> TempSharedTcp = nullptr;
    for (i = 0; i < EventNum; i++) {
        event = EventList[i].events;
        //多线程如果此处关闭了此服务器，则会出现段错误，访问了不能访问的地址。
        //如当正在接受连接时，用户操作close了这个server将发生段错误。
        //目前想到的解决方法是根据返回时的event.data.fd将等于发生传输的sockfd，根据此sockfd尝试在mtcplist中找出此通道。
        sockfd = EventList[i].data.fd;
        //printf("sockfd = %d\n", sockfd);
        TempTcp = mTcpList->GetBySocket(sockfd);
        if (!TempTcp || !TempTcp.get()) {
            //printf("[AutoTestDebug]::Error,CTCP donnot exist\n");
            continue;
        }
        dwChannel = TempTcp->GetChannel();
        HandleRet = TempTcp->NewOnEvent(event);
        switch (HandleRet) {
            case MCU_RECEIVE_CHANNEL:
                //TempTcp现在是Server
                //todo:通道号管理的一些问题，将导致mTcpList中存在两个或更多具有同样的dw_Channel的通道存在，导致GetByChannelNum时出错
                //printf("MCU_RECEIVE_CHANNEL\n");
                gChannelManager->lock();
                //printf("Handle:gChannelManager Locked\n");
                while (!gChannelManager->IsTransChannelAvailable(++dwChannel)) {
                    if ((MAXCHANNELNUM - 1) == dwChannel) {
                        //不要越界访问,不存在满足要求的通道号，则断开连接
                        gChannelManager->SetChannelStat(dwChannel, ServerTransChannel);
                        gChannelManager->unlock();
                        TempSharedTcp = NewCopyTcp(TempTcp, dwChannel);
                        NewCloseTcpChannel(dwChannel);
                        return;
                    }
                }
                //找到可用通道号，告诉拷贝函数
                gChannelManager->SetChannelStat(dwChannel, ServerTransChannel);
                gChannelManager->unlock();
                //printf("Handle:gChannelManager unLocked\n");
                

                TempSharedTcp = NewCopyTcp(TempTcp, dwChannel);
                TempSharedTcp->EpollEventsManager(TempSharedTcp->EpollFd, TempSharedTcp->mLocalSock,
                                                  EPOLL_CTL_ADD, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET,
                                                  TempSharedTcp.get());//221030:添加了EPOLLRDHUP事件
                if (m_pReceiveMessageCallback) {
                    //printf("Ready for Call Back...Start New Channel Generated\n");
                    m_pReceiveMessageCallback(TempSharedTcp->m_dwChannel, true);
                } else if (m_pChannelOnOffCallbackForThirdParty != nullptr) {
                    m_pChannelOnOffCallbackForThirdParty(TempSharedTcp->m_dwChannel, true);
                }
                //printf("Generated New CHANNEL ,ChannelNum is %d\n", TempSharedTcp->m_dwChannel);
                break;
            case ADD_NEW_TCP_CHANNEL:
                //TODO:2022/12/01
                /*这里是偷懒的做法，随后测试结束时改掉，因为没有把Connect集成到Epoll的事件处理中，现在是通过注册时会触发一次Epoll
                 * OUT事件的特点，来实现客户端通道建立连接时调用回调，在上层创建通道的操作*/
                if (ClientChannel != gChannelManager->GetChannelStat(dwChannel))break;
                if (m_pReceiveMessageCallback) {
                    //printf("Ready for Call Back...New TransChannel {%d} Generated\n", dwChannel);
                    m_pReceiveMessageCallback(dwChannel, true);
                } else if (m_pChannelOnOffCallbackForThirdParty != nullptr) {
                    m_pChannelOnOffCallbackForThirdParty(dwChannel, true);
                }
                break;
            case RX_CHANNEL_READY:
                //printf("RX_CHANNEL_READY\n");
                break;
            case TX_CHANNEL_READY:
                //printf("======TX_CHANNEL_READY======\n");
                //通知上层有通道建立
                /*if (m_pReceiveMessageCallback != nullptr) {
                    m_pReceiveMessageCallback(dwChannel, true);
                } else if (m_pChannelOnOffCallbackForThirdParty != nullptr) {
                    m_pChannelOnOffCallbackForThirdParty(dwChannel, true);
                }*/
                break;
            case TX_RX_CHANNEL_CLOSE:
                printf("TX_RX_CHANNEL_CLOSE\n");
                TempTcp->EpollEventsManager(TcpEpollFd, TempTcp->mAcceptSock, EPOLL_CTL_DEL,
                                            EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET, TempTcp.get());
                /*向上层发送通道关闭的消息，随后我觉得这些，新通道的建立和关闭，其通知和回调可以尝试集成到通道管理的ChannelManager中
                 * 尽管目前ChannelManager还非常不成熟，其机制需要重新调整，可以仿照linux分配端口的机制进行改进.
                 * */
                if (m_pReceiveMessageCallback) {
                    m_pReceiveMessageCallback(dwChannel, false);
                } else if (m_pChannelOnOffCallbackForThirdParty != nullptr) {
                    m_pChannelOnOffCallbackForThirdParty(dwChannel, false);
                }
                NewCloseTcpChannel(dwChannel);
                //printf("TcpChannel {%d} be Closed\n", dwChannel);
                break;
            default:
                //printf("default\n");
                break;
        }
    }
}

bool CNetCommEpoll::WriteUdpChannel(DWORD Channel, const std::shared_ptr<BYTE> &sendbuf, DWORD length) {
    std::shared_ptr<CUDP> TempUdp = mUdpList->GetByChannelNum(Channel);
    if (TempUdp) {
        bool ret = TempUdp->udp_write(sendbuf, length);
        /*if(ret){
            //TEST:测试视频传输
            fwrite(sendbuf.get(),length,1,fp_channel_send);
        }*/
        return ret;
    }
    return false;
}

void CNetCommEpoll::HandleUdpEvents(epoll_event *udpEvents, int EventNum) {
    if (!m_bInit) {
        return;
    }
    int i = 0;
    long HandleRet = 0;
    DWORD dwChannel = 0;
    std::shared_ptr<CUDP> TempUdp = nullptr;
    for (i = 0; i < EventNum; i++) {
        TempUdp = mUdpList->GetBySocket(udpEvents[i].data.fd);
        if (!TempUdp) {
            //printf("Error:CUDP donnot exist\n");
            continue;
        }
        dwChannel = TempUdp->GetChannel();
        HandleRet = TempUdp->OnEvent(udpEvents[i].events);
        switch (HandleRet) {
            case RX_CHANNEL_READY:
                //printf("UDP服务端建立,通道号{%d}\n", dwChannel);
                break;
            case TX_CHANNEL_READY:
                //printf("UDP通道建立,通道号{%d}\n", dwChannel);
                break;
            case TX_RX_CHANNEL_CLOSE:
                CloseUdpChannel(dwChannel);
                //printf("UDP通道关闭,通道号{%d}\n", dwChannel);
                break;
            default:
                break;
        }
    }
}

/*BCAST的函数
 * 包括：
 * CreateBCast
 * GetBCast
 * DeleteBCast
 * HandleBCastEvent etc.*/

std::shared_ptr<CBCast> CNetCommEpoll::CreateBCast() {
    std::shared_ptr<CBCast> TempBCast = std::make_shared<CBCast>(BCastEpollFd, m_pReceiveDataCallback);
    if (TempBCast) {
        mBcastList->AddTail(TempBCast);
    }
    return TempBCast;
}

std::shared_ptr<CBCast> CNetCommEpoll::GetBCast(DWORD Channel) const {
    std::shared_ptr<CBCast> TempBCast = mBcastList->GetByChannelNum(Channel);
    if (TempBCast) {
        return TempBCast;
    }
    return nullptr;
}

void CNetCommEpoll::DeleteBCast(const std::shared_ptr<CBCast> &TempBCast) const {
    if (TempBCast) {
        mBcastList->RemoveAt(TempBCast);
    }
}

bool CNetCommEpoll::WriteBCastChannel(DWORD Channel, std::shared_ptr<BYTE> sendbuf, DWORD length) const {
    std::shared_ptr<CBCast> TempBCast = mBcastList->GetByChannelNum(Channel);
    if (TempBCast) {
        return TempBCast->bcast_write(std::move(sendbuf), length);
    }
    return false;
}

void CNetCommEpoll::HandleBCastEvents(epoll_event *bcastEvents, int EventNum) const {
    if (!m_bInit) {
        return;
    }
    int i = 0;
    int SockFd = 0;
    long HandleRet = 0;
    DWORD dwChannel = 0;
    CBCast *TempBCast = nullptr;
    for (i = 0; i < EventNum; i++) {
        TempBCast = (CBCast *) bcastEvents[i].data.ptr;
        SockFd = TempBCast->mAcceptSock;
        if (!TempBCast) {
            //printf("Error:CBCast donnot exist\n");
            continue;
        }
        dwChannel = TempBCast->GetChannel();
        HandleRet = TempBCast->OnEvent(SockFd, bcastEvents[i].events);
        switch (HandleRet) {
            case RX_CHANNEL_READY:
                break;
            case TX_CHANNEL_READY:
                break;
            case TX_RX_CHANNEL_CLOSE:
                break;
            default:
                break;
        }
    }
}

void CNetCommEpoll::SetInterfaceParam(PMESSAGECALLBACK pMsgCallback, PMESSAGECALLBACK pMsgCallback2, PDATACALLBACK pDataCallback) {
    m_pReceiveDataCallback[0] = pDataCallback;
    m_pReceiveMessageCallback = pMsgCallback;
    m_pChannelOnOffCallbackForCombTransfer = pMsgCallback2;
}

bool CNetCommEpoll::DoRegister_NetCommCallBack(PDATACALLBACK pDataCallback) {
    for (UINT i = 1; i < 10; i++) {
        if (nullptr == m_pReceiveDataCallback[i]) {
            m_pReceiveDataCallback[i] = pDataCallback;
            return true;
        }
    }
    return false;
}

void CNetCommEpoll::CloseAllSocket() const {
    mTcpList->NetChannel.clear();
    mUdpList->NetChannel.clear();
    mBcastList->NetChannel.clear();
}
