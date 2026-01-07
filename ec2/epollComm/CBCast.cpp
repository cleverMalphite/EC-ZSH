//
// Created by 王炳棋 on 2022/10/31.
//
#include "CBCast.h"
#include "CNetCommEpoll.h"

#include "infoHub/rateStatistic.h"

extern CNetCommEpoll *gNetCommManager;

extern ec2::rateStatistic   rx_rate_stat;

CBCast::CBCast(int EpollFd, PDATACALLBACK *pReceiveDataCallback) :
        BCastEpollFd(EpollFd),
        m_bInit(false),
        m_bCanRead(false),
        m_bCanWrite(false),
        m_bConnected(false),
        m_bIsServer(false),
        m_bStartRead(true),
        m_bIsFirstWriteMessage(true),
        m_pReceiveDataCallback(pReceiveDataCallback) {
    //
}

CBCast::~CBCast() {
    m_bStartRead = false;
    m_bInit = false;
    m_bConnected = false;
    m_bCanRead = false;
    m_bCanWrite = false;
    FreeAllMemory();
}

void CBCast::FreeAllMemory() {
    //todo:释放临时存储的buffer队列

}

void CBCast::bcast_init(UINT nBCastPort, std::string szLocalIPAddress, std::string szBCastAddr) {
    //SetLocalPort(nBCastPort);
    //为了在一台机器上测试，所以使用随机端口绑定bcast的套接字
    SetLocalPort(0);
    SetRemotePort(nBCastPort);//广播端口号=接收方接收端口号
    SetLocalAddress(szLocalIPAddress);
    SetRemoteAddress(szBCastAddr);
}

bool CBCast::bcast_open(bool isServer) {
    m_bIsServer = isServer;
    bool ret = false;
    if (m_bInit)return true;

    ret = net_open(SOCK_DGRAM);
    if (!ret) {
        return false;
    }
    int bcast = 1;
    int result = setsockopt(mLocalSock, SOL_SOCKET, SO_BROADCAST, (void *) &bcast, sizeof(bcast));
    if (0 < result) {
        //BCastErr = errno;
        perror("set broad cast");
        return false;
    }
    ret = net_bind();
    if (!ret) {
        return false;
    }
    if (!isServer) {
        ret = net_connect();//为什么调用connect？猜测是为了过滤某个特定地址的广播？connect可以过滤udp收到的数据。
        if (!ret) {
            return false;
        }
    }
    ret = EpollEventsManager(BCastEpollFd, mAcceptSock, EPOLL_CTL_ADD, EPOLLIN | EPOLLET, this);
    if (!ret) {
        return false;
    }
    m_bInit = true;
    return m_bInit;
}

bool CBCast::bcast_close() {
    bool ret;
    m_bStartRead = false;
    //删除监听事件
    ret = EpollEventsManager(BCastEpollFd, mAcceptSock, EPOLL_CTL_DEL, EPOLLIN | EPOLLET, this);
    net_disconnect_udp();
    ret = net_disconnect();

    m_bInit = false;
    m_bConnected = false;
    m_bCanRead = false;
    m_bCanWrite = false;
    return ret;
}

std::shared_ptr<BYTE> CBCast::bcast_read(unsigned long &Length) {
    std::shared_ptr<BYTE> ret = net_UdpRecv(Length);
    
    //------ infohub --------
    rx_rate_stat.pass(Length);

    return ret;
}

bool CBCast::bcast_write(std::shared_ptr<BYTE> buf, int Length) {
    //master分支下，会判断mbcanwrite状态量的值，如果可写就写，不可写就将返回false，在得到写事件时再将其设置为true，对于udpepoll写事件将一直触发或仅触发一次
    //所以认为不需要此状态量，不过需要注意非阻塞的socket在send时返回的错误。
    int ret = net_sendTo(buf, Length);
    if (0 > ret) {
        //error handle
        perror("Bcast Send Error");
        return false;
    } else {
        return true;
    }
}

long CBCast::OnEvent(int SockFd, uint32_t Events) {
    int Socket = SockFd;
    long EventRet = 0;
    unsigned long Length = 0;
    std::shared_ptr<BYTE> pData = nullptr;
    if (Events & EPOLLOUT) {
        m_bCanWrite = true;
        if (m_bIsFirstWriteMessage) {
            m_bIsFirstWriteMessage = false;
            if (m_bIsServer) {
                EventRet = RX_CHANNEL_READY;
            } else {
                EventRet = TX_CHANNEL_READY;
            }
        }
    }
    if (Events & EPOLLIN) {
        if (!m_bStartRead) return EventRet;
        m_bCanRead = true;
        while (RecvERRNO != EAGAIN) {
            pData = bcast_read(Length);
            if (pData && Length) {
                printf("bcast server read:%s\n", pData.get());
                if (m_dwChannel >= RESERVE_CHANNEL_BEGIN && m_dwChannel <= RESERVE_CHANNEL_END) {
                    //如果确实有数据读入，把此事件通报上层注册的回调函数，g_pWndSocketDlg是原NetComm中的定义在NetCommApi中的一个类似于新NetComm中的CNetCommEpoll结构。
                    if (nullptr != gNetCommManager->m_pReceiveDataForThirdPartyTransfer) {
                        gNetCommManager->m_pReceiveDataForThirdPartyTransfer(m_dwChannel, pData, Length, false);
                    }
                } else if (m_pReceiveDataCallback) {
                    for (int i = 0; i < GLOBAL_MAX_REGISTER_CALLBACK_FUN; i++) {
                        if (m_pReceiveDataCallback + i) {
                            m_pReceiveDataCallback[i](m_dwChannel, pData, Length, false);
                        }
                    }
                } else {
                    /*pthread_mutex_lock(&bcastMutex);
                    if (bufferIndex < UDP_MAX_PACKET_NUMBER) {
                        BcastBuffers[bufferIndex].pBuffer = pData;
                        BcastBuffers[bufferIndex].uSocket = Socket;
                        BcastBuffers[bufferIndex].dwLength = Length;
                        bufferIndex++;
                    } else {
                        bufferIndex = 0;
                    }
                    pthread_mutex_lock(&bcastMutex);*/
                }
                Length = 0;
                pData = nullptr;
            }
        }
        RecvERRNO = 0;
        if (m_bConnected) {

        } else if (m_bInit) {
            if (m_bIsServer) {
                EventRet = TX_CHANNEL_READY;    //对UDP连接来说，有读事件时对服务器来说是可写，对客户端来说是可读
                m_bConnected = true;
            } else {
                EventRet = RX_CHANNEL_READY;
                m_bConnected = true;
            }
        }
    }
    return EventRet;
}
