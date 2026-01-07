//
// Created by 王炳棋 on 2022/10/4.
//

#include "CUDP.h"

#include "infoHub/rateStatistic.h"

#include <utility>

extern ec2::rateStatistic   rx_rate_stat;

CUDP::CUDP() {

}

CUDP::CUDP(int EpollFd, PDATACALLBACK *pReceiveDataCallback, bool canRead, bool canWrite)
        : UdpEpollFd(EpollFd),
          bufferIndex(0),
          m_bInRead(false),
          m_bCanRead(canRead),
          m_bCanWrite(canWrite),
          m_bIsServer(false),
          m_bIsBothWay(false),
          m_bStartRead(true),
          m_bIsFirstWriteMessage(true),
          m_pReceiveDataCallback(pReceiveDataCallback),
          UdpBuffers(std::make_unique<NetBuffer[]>(UDP_MAX_PACKET_NUMBER)) {
    if (canRead && canWrite) {
        m_bIsBothWay = true;
    }
    pthread_mutex_init(&udpMutex, nullptr);
    //printf("CUDP Created\n");

}

CUDP::~CUDP() {
    //todo:EpollEvent delete
    m_bStartRead = false;
    m_bInit = false;
    m_bConnected = false;
    m_bCanRead = false;
    m_bCanWrite = false;
    //m_bIsBothWay = FALSE;
    //m_nChannelType = 0;
    FreeAllMemory();
    pthread_mutex_destroy(&udpMutex);
}

void CUDP::FreeAllMemory() {
    m_bStartRead = false;
    //释放临时存储数据的数据空间
    UdpBuffers = nullptr;
}

void CUDP::Restart() {

}

void CUDP::udp_init(UINT nLocalPort, UINT nRemotePort, std::string szLocalAddress, const std::string &szRemoteAddress) {
    SetLocalPort(nLocalPort);
    SetRemotePort(nRemotePort);
    SetLocalAddress(std::move(szLocalAddress));
    if (!szRemoteAddress.empty()) {
        SetRemoteAddress(szRemoteAddress);
    }
}

bool CUDP::udp_open(bool IsServer) {
    m_bIsServer = IsServer;

    bool ret = false;
    if (m_bInit) {
        return false;
    }
    ret = net_open(SOCK_DGRAM);
    if (!ret) return false;
    ret = net_bind();
    if (!ret) return false;

    if (!IsServer) {
        //接收广播的套接字应该不需要建立连接叭
        ret = net_connect();
        if (!ret) return false;
    }
    //测试：注册了
    ret = EpollEventsManager(UdpEpollFd, mAcceptSock,
                             EPOLL_CTL_ADD, EPOLLIN | EPOLLRDHUP | EPOLLET, this);
    m_bInit = true;
    m_bCanRead = true;
    m_bCanWrite = true;
    //printf("UDP OPEN SUCCESS\n");
    return true;
}

bool CUDP::udp_close() {
    //todo：重写
    bool ret;
    m_bStartRead = false;
    //ret = EpollEventsManager();
    //net_disconnect_udp();
    ret = net_disconnect();
    m_bInit = false;
    m_bConnected = false;
    m_bCanWrite = false;
    m_bCanRead = false;
    return false;
}

std::shared_ptr<BYTE> CUDP::udp_read(unsigned long &length) {
    std::shared_ptr<BYTE> buffer = nullptr;
    length = 0;
//    buffer = net_Recv(length);
    buffer = net_UdpRecv(length);

    /*//TEST:测试视频传输
    if(buffer.get()!= nullptr){
        fwrite(buffer.get(),length,1,fp_channel_recv);
    }*/

    if (!buffer) {
        if (errno != EAGAIN) {
            //printf("errno:%d, ", errno);
            perror("udp read");
        }
        if (m_dwChannel >= RESERVE_CHANNEL_BEGIN && m_dwChannel <= RESERVE_CHANNEL_END) {
            return nullptr;
        } else {
            //todo:Restart()
            Restart();
            return nullptr;
        }
    }
    if (!buffer) m_bCanRead = false;
    
    //--------- infohub ----------
    rx_rate_stat.pass(length);

    return buffer;
}

bool CUDP::udp_write(const std::shared_ptr<BYTE>& buf, int length) {
    int ret = 0;
    //if (!m_bCanWrite)return false;
    

    ret = net_Send(buf, length);

    //TEST:测试视频传输
    //printf("udpSendRet is %d\n",ret);
    if (0 > ret) {
        if (m_dwChannel >= RESERVE_CHANNEL_BEGIN && m_dwChannel <= RESERVE_CHANNEL_END) {

        } else {
            if (errno == EAGAIN) {
                while (0 > ret) {
                    ret = net_Send(buf, length);
                }
//                fwrite(buf.get(),length,1,fp_channel_send);
            } else {
                Restart();
            }
        }
        return false;
    } else {
//        fwrite(buf.get(),length,1,fp_channel_send);
        return true;
    }
}

void CUDP::SetUdpServerChannelRemoteAddress(const std::string& remote_address, DWORD remote_port) {
    printf("CUDP------------------1---------------------+\n");
    printf("CUDP SetUdpServerChannelRemoteAddress ip:%s port:%d\n", remote_address.c_str(), remote_port);
    printf("+-------------------------------------------+\n");

    mRemoteAddr.sin_family = AF_INET;
    mRemoteAddr.sin_port = htons(remote_port);
    mRemoteAddr.sin_addr.s_addr = inet_addr(remote_address.c_str());
    mRemoteAddress = remote_address;
    mRemotePort = remote_port;
}

long CUDP::OnEvent(uint32_t Events) {
    //todo：思考是否把EventRet改成类似epolldata中events的形式，当存在一次Events中有多个EPOLL事件时，现在只能同时返回一种状态。
    long EventRet = 0;
    unsigned long Length = 0;
    std::shared_ptr<BYTE> pData = nullptr;

    if (Events & EPOLLRDHUP) {
        //printf("[NetCommDebug]::EPOLLRDHUP,UDP Connection is close\n");
        EventRet = TX_RX_CHANNEL_CLOSE;
        return EventRet;
    }
    if (Events & EPOLLERR) {
        //printf("[NetCommDebug]::EPOLLERR,UDP Connection is close\n");
        EventRet = TX_RX_CHANNEL_CLOSE;
        return EventRet;
    }
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
        if (!m_bConnected && m_bInit) {
            if (m_bIsServer) {
                EventRet = TX_CHANNEL_READY;    //对UDP连接来说，有读事件时对服务器来说是可写，对客户端来说是可读
                m_bConnected = true;
            } else {
                EventRet = RX_CHANNEL_READY;
                m_bConnected = true;
            }
        }
        m_bCanRead = true;
        //todo:UDP_ET_READ
        //if (m_bInRead) return EventRet;//实验发现不能加此状态量，当sock的接收缓冲在inread还没有调整为false时被填满，将导致无法再触发EpollIn的现象
        //m_bInRead = true;
        pData = udp_read(Length);
        while (/*RecvERRNO != EAGAIN*/pData) {
            /*pData = udp_read(Length);*/
            if (pData && Length) {
                if (m_dwChannel >= RESERVE_CHANNEL_BEGIN && m_dwChannel <= RESERVE_CHANNEL_END) {
                    //如果确实有数据读入，把此事件通报上层注册的回调函数，g_pWndSocketDlg是原NetComm中的定义在NetCommApi中的一个类似于新NetComm中的CNetCommEpoll结构。
                    /*if (nullptr != m_pReceiveDataForThirdPartyTransfer) {
                        m_pReceiveDataForThirdPartyTransfer[0](m_dwChannel, pData, Length, false);
                    }*/
                    //printf("[NetCommDebug]::This Channel is a Reserve Channel\n");
                } else if (m_pReceiveDataCallback) {
//                    std::cout<<"[CUDP] OnEvent: recved data"<<std::endl;
                    /*for (int i = 0; i < GLOBAL_MAX_REGISTER_CALLBACK_FUN; i++) {
                        if (m_pReceiveDataCallback + i) {
                            m_pReceiveDataCallback[i](m_dwChannel, pData, Length, false);
                        }
                    }*/
                    /*//TEST:测试视频传输
                    fwrite(pData.get(),Length,1,fp_channel_recv);*/

                    m_pReceiveDataCallback[0](m_dwChannel, pData, Length, false);
                } else {
                    pthread_mutex_lock(&udpMutex);
                    if (bufferIndex < UDP_MAX_PACKET_NUMBER) {
                        UdpBuffers[bufferIndex].pBuffer = pData;
                        UdpBuffers[bufferIndex].uSocket = mAcceptSock;
                        UdpBuffers[bufferIndex].dwLength = Length;
                        bufferIndex++;
                    } else {
                        bufferIndex = 0;
                    }
                    pthread_mutex_unlock(&udpMutex);
                }
                Length = 0;
                pData = nullptr;
            }
            pData = udp_read(Length);
        }
        /*RecvERRNO = 0;*/
        //m_bInRead = false;//todo:m_bInRead是否有用？无效，不能加
    }
    return EventRet;
}
