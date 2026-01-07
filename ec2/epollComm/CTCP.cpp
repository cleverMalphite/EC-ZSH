//
// Created by 王炳棋 on 2022/9/29.
//
#include "CTCP.h"

#include "./tcpSocketInfoTool.h"

#include "infoHub/infoHub.h"
#include "infoHub/rateStatistic.h"
#include "infoHub/rateStatisticTable.h"

#include <utility>

extern std::shared_ptr<ec2::infoHub>\
            _epollcomm_infohub_instance;

extern ec2::rateStatistic   rx_rate_stat;

extern ec2::rateStatisticTable rx_cid_rate_sttable;
extern ec2::rateStatisticTable tx_cid_rate_sttable;

CTCP::CTCP() {

    EpollFd = 0;

    m_bIsServer = false;
    m_bConnected = false;
    m_bCanRead = false;
    m_bCanWrite = false;
    //这个成员改为m_bOpenReady  m_bInit = false;
    m_bOpenReady = false;
    m_bInRead = false;
    mLocalSock = 0;
    mAcceptSock = 0;
    m_bReceiveChannel = false;
    m_bIsFirstWriteMessage = true;
    m_dwChannelNumber = 0;
    m_nChannelType = TCPChannel;  //表示是tcp链路
    m_bReadyForKeepAliveCheck = false;

    m_pReceiveDataCallback = nullptr;
    pthread_mutex_init(&tcpmtx, nullptr);
}

CTCP::CTCP(int TcpEpollFd, PDATACALLBACK *pReceiveDataCallback) {

    EpollFd = TcpEpollFd;

    m_bIsServer = false;
    m_bConnected = false;
    m_bCanRead = false;
    m_bCanWrite = false;
    //这个成员改为m_bOpenReady  m_bInit = false;
    m_bOpenReady = false;
    m_bInRead = false;
    mLocalSock = 0;
    mAcceptSock = 0;
    m_bReceiveChannel = false;
    m_bIsFirstWriteMessage = true;
    m_dwChannelNumber = TcpMaxChannelNum;
    m_nChannelType = TCPChannel;  //表示是tcp链路
    m_bReadyForKeepAliveCheck = false;

    m_pReceiveDataCallback = pReceiveDataCallback;
    pthread_mutex_init(&tcpmtx, nullptr);
    //printf("One CTCP Created\n");
}

CTCP::~CTCP() {
    m_bConnected = false;
    m_bCanRead = false;
    m_bCanWrite = false;
    m_bOpenReady = false;
    m_nChannelType = 0;
    pthread_mutex_destroy(&tcpmtx);
    FreeAllMemory();
}

void CTCP::FreeAllMemory() {
    /*while(mBufferList.IsEmpty()){
        need to free;
    }*/
    while (!mChannelList.IsEmpty()) {
        //shareptr自动释放内存,todo:test
        mChannelList.RemoveHead();
    }
}

bool CTCP::tcp_isMCU() const {
    if (2 <= m_dwChannelNumber) {
        return true;
    } else {
        return false;
    }
}

bool CTCP::isAllChannelListenOver() const {
    if (m_dwChannelCurrentNumber >= m_dwChannelNumber - 1) {
        return true;
    } else {
        return false;
    }
}

void CTCP::tcp_init(UINT localPort, UINT remotePort, std::string localAddress, std::string remoteAddress) {
    printf("CTCP------------------------------------\n");
    std::cout<<"localAddr:"<<localAddress<<std::endl<<"localPOrt:"<<localPort<<std::endl;
    printf("----------------------------------------\n");
    SetLocalPort(localPort);
    SetRemotePort(remotePort);
    SetLocalAddress(std::move(localAddress));
    SetRemoteAddress(std::move(remoteAddress));
}

bool CTCP::tcp_open(bool IsServer) {
    m_bIsServer = IsServer;
    m_bReceiveChannel = true;
    /*m_bReceiveChannel = IsServer;*/

    bool ret = false;
    if (m_bOpenReady)return true;
    ret = net_open(SOCK_STREAM); //net_open 创建sock并设置sockbuf
    if (!ret) {
//        printf("netopen\n");
        perror("netopen");
        return false;
    }
    if (IsServer) {
        ret = net_bind();
        if (!ret) {
//            printf("netbind\n");
            perror("netbind");
            return false;
        }
        /* 监听使用LT模式 */
        EpollEventsManager(EpollFd, mLocalSock, EPOLL_CTL_ADD, EPOLLIN, this);
        ret = net_listen(5);
        if (!ret) {
//            printf("netlisten\n");
            perror("netlisten");
            return false;
        }
//        printf("Start Listen Connection\n");
    } else {
        ret = net_bind();
        if (!ret) {
//            printf("netbind\n");
            perror("netbind");
            return false;
        }
        //printf("Start Connect on Server\n");
        ret = net_connect();
        if (!ret) {
//            printf("netconnect\n");
            perror("netconnect");
            return false;
        }
        m_bConnected = true;
        EpollEventsManager(EpollFd, mLocalSock, EPOLL_CTL_ADD, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET, this);
    }
    m_bOpenReady = true;
    return true;
}

DWORD CTCP::GetNextChannel() {

    m_dwChannelCurrentNumber++;
    //printf("New Connection,m_dwChannelCurrentNumber++,is %d\n", m_dwChannelCurrentNumber);
    //server当前管理的通道数+1
    //从通道列表尾取节点
    mChannelList.Lock();
    std::shared_ptr<NODE> pTail = mChannelList.GetTail();
    std::shared_ptr<NODE> pNode = std::make_shared<NODE>();
    //创建新的节点并设置其通道值顺序加1
    //如果pTail为空，那么返回-1
    if (!pTail || (pTail && (nullptr == pTail.get()))) {
        return -1;
    }
    pNode->ChannelNumber = pTail->ChannelNumber + 1;
    mChannelList.AddTail(pNode);
    //printf("New Channel node{%d}\n", pNode->ChannelNumber);
    mChannelList.UnLock();
    //原dwchannel值为最新加入的clientchannel，server的channelnum+1；
    m_dwChannel++;
    //printf("Now ServerChannelNum is %d\n", m_dwChannel);
    return m_dwChannel;
}

//创建TCPServer时，会调用，其中ChannelHead应该是用来监听的TCP通道
//todo:needTest:20221026
void CTCP::SetChannel_TcpList(DWORD ChannelHead, DWORD ChannelNumber) {
    std::shared_ptr<NODE> pNode = std::make_shared<NODE>();
    pNode->ChannelNumber = ChannelHead;
    SetChannel(pNode->ChannelNumber);
    mChannelList.AddTail(pNode);
    //m_dwChannel = ChannelHead;
    m_dwChannelCurrentNumber = 0;    //当前接入数
    m_dwChannelNumber = ChannelNumber;    //最大接入数
    
}


bool CTCP::tcp_newCopy(const std::shared_ptr<CTCP>& TcpServer, DWORD dwChannel) {
    if (!TcpServer) {
        return false;
    }
    EpollFd = TcpServer->EpollFd;
    m_bConnected = TcpServer->m_bConnected;
    mLocalSock = TcpServer->mLocalSock;
    mRemoteAddress = TcpServer->mRemoteAddress;
    mLocalAddress = TcpServer->mLocalAddress;
    mRemotePort = TcpServer->mRemotePort;
    mRemoteAddr = TcpServer->mRemoteAddr;
    mAcceptAddr = TcpServer->mAcceptAddr;
    mAcceptSock = TcpServer->mAcceptSock;
    mLocalSock = TcpServer->mAcceptSock;
    TcpServer->mAcceptSock = TcpServer->mLocalSock;
    //新的服务器连接通道号设置机制
    
    //dtu channel set
    SetIsDTUChannel( TcpServer->GetIsDTUChannel() );

    //m_dwChannel = dwChannel;
    SetChannel(dwChannel);

    {
        TcpServer->m_dwChannelCurrentNumber++;
        std::shared_ptr<NODE> pNode = std::make_shared<NODE>();
        pNode->ChannelNumber = dwChannel;
        TcpServer->mChannelList.AddTail(pNode);
    }
    TcpServer->m_bIsFirstWriteMessage = true;
    TcpServer->m_bCanWrite = false;

    //m_dwChannelCurrentNumber = TempTcp->m_dwChannelCurrentNumber + 1;
    return true;
}

bool CTCP::tcp_close() {
    //printf("[AutoTestDebug]::关闭TCP连接\n");
    bool ret;
    if (!m_bIsServer) {
        //ret =
    }
    ret = net_disconnect();

    if (ret) {
        m_bConnected = false;
        m_bCanRead = false;
        m_bCanWrite = false;

        if (!m_bIsServer) {
            m_bOpenReady = false;
        }
    }
    return ret;
}

bool CTCP::tcp_write(const std::shared_ptr<BYTE>& buf, int length) {
    bool ret = false;
    pthread_mutex_lock(&tcpmtx);
    if (!m_bConnected) {
        //printf("Connection has not set up or broken\n");
        pthread_mutex_unlock(&tcpmtx);
        return ret;
    }
    if (!m_bCanWrite) {
        pthread_mutex_unlock(&tcpmtx);
        return ret;
    }

    if (!buf.get() || !length) {
        pthread_mutex_unlock(&tcpmtx);
        return true;
    }

    int nLeft = length;
    int idx = 0;
    int SendNum = 0;
    while (0 < nLeft) {
        std::shared_ptr<BYTE> byteSharedPtrTemp(new BYTE[nLeft], releaseArrays<BYTE>);
        memcpy(byteSharedPtrTemp.get(), buf.get() + idx, nLeft);
        SendNum = net_Send(byteSharedPtrTemp, nLeft);
        if (0 > SendNum) {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                //perror("send");
                m_bConnected = false;
            }
            break;
        }
        else {
            //-----tcp info for infohub-----
            tcpSocketInfoTool tcp_info(mAcceptSock);
            m_rtt = tcp_info.get_rtt();
            m_loss = tcp_info.get_loss();
            _epollcomm_infohub_instance->table_set("epollcomm", "qos_tx_cid_rtt_sttable", GetChannel(), (uint32_t)m_rtt);
            _epollcomm_infohub_instance->table_set("epollcomm", "qos_tx_cid_loss_sttable", GetChannel(), (uint32_t)m_loss);

            _epollcomm_infohub_instance->value_set("epollcomm", "qos_tx_rx_rtt_stat",  (uint32_t)m_rtt);
            _epollcomm_infohub_instance->value_set("epollcomm", "qos_tx_rx_loss_stat", (uint32_t)m_loss);


        }
        nLeft -= SendNum;
        idx += SendNum;
    }
    if (0 < nLeft) {
        m_bCanWrite = false;
        std::shared_ptr<BYTE> temp(new BYTE[nLeft], releaseArrays<BYTE>);
        /*tcpMutex.lock();*/
        m_pBuffer2Send = temp;
        temp = nullptr;
        if (m_pBuffer2Send) {
            //printf("reload pBuffer2Send,length is %d\n", nLeft);
            memcpy(m_pBuffer2Send.get(), buf.get() + length - nLeft, nLeft);
            m_nBufferLenSended = 0;
            m_nBufferLength = nLeft;
        }
        /*tcpMutex.unlock();*/
        pthread_mutex_unlock(&tcpmtx);
        return false;
    } else if (0 == nLeft) {
        pthread_mutex_unlock(&tcpmtx);
        return true;
    } else {
        m_bCanWrite = false;
        pthread_mutex_unlock(&tcpmtx);
        return false;
    }
}

bool CTCP::TcpEpollWrite() {
    /*tcpMutex.lock();*/
    pthread_mutex_lock(&tcpmtx);
    if (m_pBuffer2Send) {
        int nLeft = m_nBufferLength - m_nBufferLenSended;
        int idx = m_nBufferLenSended;
        int SendNum = 0;
        while (0 < nLeft) {
            std::shared_ptr<BYTE> byteSharedPtrTemp(new BYTE[nLeft], releaseArrays<BYTE>);
            memcpy(byteSharedPtrTemp.get(), m_pBuffer2Send.get() + m_nBufferLenSended, nLeft);
            SendNum = net_Send(byteSharedPtrTemp, nLeft);
            if (SendNum < 0) {
                //printf("in sending pbuffer2send\n");
                break;
            }else{
                //-----tcp info for infohub-----
                tcpSocketInfoTool tcp_info(mAcceptSock);
                m_rtt = tcp_info.get_rtt();
                m_loss = tcp_info.get_loss();
                _epollcomm_infohub_instance->table_set("epollcomm", "qos_tx_cid_rtt_sttable", GetChannel(), (uint32_t)m_rtt);
                _epollcomm_infohub_instance->table_set("epollcomm", "qos_tx_cid_loss_sttable", GetChannel(), (uint32_t)m_loss);

                _epollcomm_infohub_instance->value_set("epollcomm", "qos_tx_rx_rtt_stat",  (uint32_t)m_rtt);
                _epollcomm_infohub_instance->value_set("epollcomm", "qos_tx_rx_loss_stat", (uint32_t)m_loss);
            }
            nLeft -= SendNum;
            m_nBufferLenSended += SendNum;
            SendNum = 0;
        }

        if (nLeft) {
            /*tcpMutex.unlock();*/
            pthread_mutex_unlock(&tcpmtx);
            return false;
        }

        m_pBuffer2Send = nullptr;
        m_nBufferLength = 0;
        m_nBufferLenSended = 0;
        /*tcpMutex.unlock();*/
        pthread_mutex_unlock(&tcpmtx);
        m_bCanWrite = true;
        return true;
    }
    /* m_bCanWrite = true;//是否要加此条件？*/
    /* tcpMutex.unlock();*/
    pthread_mutex_unlock(&tcpmtx);
    //printf("EpollWrite run once , now mbCanWrite is %d",m_bCanWrite);
    return true;
}

std::shared_ptr<BYTE> CTCP::tcp_read(unsigned long &length) {
    std::shared_ptr<BYTE> buffer = net_Recv(length);
    if (nullptr == buffer) {
        /*if(RecvERRNO != EAGAIN&&RecvERRNO != EWOULDBLOCK&&RecvERRNO!=EINTR){
            //printf("Connection may broken\n");
        }*/
        //printf("Read No Data...\n");
        m_bCanRead = false;
    }

    //-----tcp info for infohub-----
    tcpSocketInfoTool tcp_info(mAcceptSock);
    m_rtt = tcp_info.get_rtt();
    m_loss = tcp_info.get_loss();
    _epollcomm_infohub_instance->table_set("epollcomm", "qos_rx_cid_rtt_sttable", GetChannel(), (uint32_t)m_rtt);
    _epollcomm_infohub_instance->table_set("epollcomm", "qos_rx_cid_loss_sttable", GetChannel(), (uint32_t)m_loss);

    _epollcomm_infohub_instance->value_set("epollcomm", "qos_tx_rx_rtt_stat",  (uint32_t)m_rtt);
    _epollcomm_infohub_instance->value_set("epollcomm", "qos_tx_rx_loss_stat", (uint32_t)m_loss);

    //---- infohub ----
    rx_rate_stat.pass(length);

    return buffer;
}

long CTCP::NewOnEvent(uint32_t Events) {
    long EventRet = 0;
    unsigned long Length = 0;
    std::shared_ptr<BYTE> pData = nullptr;
    std::shared_ptr<NetBuffer> pReadBuffer = nullptr;


    if (Events & EPOLLRDHUP) {
        //printf("[NetCommDebug]::EPOLLRDHUP,TCP Connection is close\n");
        EventRet = TX_RX_CHANNEL_CLOSE;
        return EventRet;
    }
    if (Events & EPOLLERR) {
        //printf("[NetCommDebug]::EPOLLERR,TCP Connection is close\n");
        EventRet = TX_RX_CHANNEL_CLOSE;
        return EventRet;
    }
    if (Events & EPOLLOUT) {
        if (m_bIsFirstWriteMessage) {
            m_bCanWrite = true;
            m_bIsFirstWriteMessage = false;
            if (m_bConnected) {
                if (m_bReceiveChannel) {
                    EventRet = RX_CHANNEL_READY;
                } else {
                    EventRet = TX_CHANNEL_READY;
                }
                if (!m_bIsServer) {
                    //建立客户端通道，第一次触发EPOLLOUT;
                    EventRet = ADD_NEW_TCP_CHANNEL;
                }
            }
        } else {
            if (!TcpEpollWrite()) {
                //printf("pBuffer2Send use EpollWrite() failed\n");
            }
        }
    }
    if (Events & EPOLLIN) {
        if (!tcp_IsServer()) {
            //printf("EPOLLIN,Recving Data\n");
            m_bCanRead = true;
            if (!m_bConnected) return EventRet;
            while (m_bInRead) {
                usleep(10);
            }
            m_bInRead = true;
            Length = 0;
            pData = nullptr;
            pData = tcp_read(Length);
            if (pData.get() && Length) {
                /*//printf("reading Data\n");*/
                if (m_pReceiveDataCallback) {
                    //printf("Ready for TCPData Call Back...Data Length is %ld\n", Length);
                    m_pReceiveDataCallback[0](m_dwChannel, pData, Length, true);
                    pData = nullptr;
                    Length = 0;
                } else {
                    /*//printf("There is no CallBack...\n");*/

                    //TODO houlc-why : no meaning operation of new NetBuffer ?
                    pReadBuffer = nullptr;
                    std::shared_ptr<NetBuffer> tempBuf(new NetBuffer());
                    pReadBuffer = tempBuf;
                    if (pReadBuffer) {
                        pReadBuffer->pBuffer = pData;
                        pReadBuffer->dwLength = Length;
                        pReadBuffer->uSocket = mAcceptSock;
                        //mBufferList.AddTail(pReadBuffer);
                    } else {
                        pData = nullptr;
                        Length = 0;
                    }
                }
            } else {
                if (RecvERRNO != EAGAIN && RecvERRNO != EWOULDBLOCK && RecvERRNO != EINTR) {
                    //printf("Connection may broken\n");
                    EventRet = TX_RX_CHANNEL_CLOSE;
                }
            }
            m_bInRead = false;
            return EventRet;
        } else {
            if (!tcp_isMCU()) {
                //printf("EPOLLIN,Tcp Is Server But Only One Channel\n");
                if (!m_bConnected) {
                    if (net_accept()) {
                        m_bIsServer = false;
                        EpollEventsManager(EpollFd, mAcceptSock, EPOLL_CTL_ADD, EPOLLIN | EPOLLOUT | EPOLLET,
                                           this);
                        EpollEventsManager(EpollFd, mLocalSock, EPOLL_CTL_DEL, EPOLLIN, this);
                    }
                }
                m_bConnected = true;
                m_bIsServer = false;
                if (m_bCanWrite) {
                    EventRet = RX_CHANNEL_READY;
                }
            } else if (isAllChannelListenOver()) {
                //printf("EPOLLIN,Tcp Is Server But Rufusing New Connection\n");
                net_refuse();
            } else {
                //printf("MCU Recving A New Channel\n");
                net_accept();
                m_bConnected = true;
                EventRet = MCU_RECEIVE_CHANNEL;
                return EventRet;
            }
        }
    }
    return EventRet;
}


void CTCP::SetIsDTUChannel(bool isDTUChannel ){
    m_IsDTUChannel = isDTUChannel;
}

bool CTCP::GetIsDTUChannel(){
    return m_IsDTUChannel;
}
