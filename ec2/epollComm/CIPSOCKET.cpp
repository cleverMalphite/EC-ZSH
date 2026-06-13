//
// Created by wangbingqi on 22-9-28.
//

#include "CIPSOCKET.h"
#include <utility>

#include "infoHub/rateStatisticTable.h"

extern ec2::rateStatisticTable rx_cid_rate_sttable;
extern ec2::rateStatisticTable tx_cid_rate_sttable;


CIP_SOCKET::CIP_SOCKET() {
    pthread_mutex_init(&sockMutex, nullptr);
}

CIP_SOCKET::~CIP_SOCKET() {
    pthread_mutex_destroy(&sockMutex);
}


UINT CIP_SOCKET::SetLocalPort(UINT Port) {
    UINT dd = mLocalPort;
    mLocalPort = Port;
    return dd;
}

UINT CIP_SOCKET::SetRemotePort(UINT Port) {
    UINT dd = mRemotePort;
    mRemotePort = Port;
    return dd;
}

DWORD CIP_SOCKET::GetLocalPort() {
    return mLocalPort;
}

std::string CIP_SOCKET::GetLocalAddress() {
    return mLocalAddress;
}

std::string CIP_SOCKET::GetRemoteAddress() {
    return mRemoteAddress;
}

std::string CIP_SOCKET::SetLocalAddress(std::string Address) {
    std::string ss = mLocalAddress;
    mLocalAddress = std::move(Address);
    return ss;
}

std::string CIP_SOCKET::SetRemoteAddress(std::string Address) {
    std::string ss = mRemoteAddress;
    mRemoteAddress = std::move(Address);
    return ss;
}

bool CIP_SOCKET::net_open(int type) { //安全，同时对一个对象执行open本身逻辑就是错误的。

    mLocalSock = socket(AF_INET, type, 0);
    if (mLocalSock <= 0) {
        printf("Create Socket Failed:%d\n", mLocalSock);
        perror("Create socket");
        return false;
    }

    mLocalAddrLen = sizeof(sockaddr_in);
    memset(&mLocalAddr, 0, mLocalAddrLen);
    mLocalAddr.sin_family = AF_INET;
    mLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    mLocalAddr.sin_port = htons(mLocalPort);

    mAcceptAddrLen = sizeof(sockaddr_in);
    memset(&mRemoteAddr, 0, mAcceptAddrLen);
    mRemoteAddr.sin_family = AF_INET;
    mRemoteAddr.sin_addr.s_addr = inet_addr(mRemoteAddress.c_str());
    mRemoteAddr.sin_port = htons(mRemotePort);

    printf("<CIPSOCKET> net open remote ip:%s\n", mRemoteAddress.c_str());
    printf("<CIPSOCKET> net open remote port:%d\n", mRemotePort);

    mAcceptAddr = mLocalAddr;
    mAcceptSock = mLocalSock;

    int socketBuffer = 1024 * 1024 * 16;
    if (SOCK_STREAM == type) {
        if (setsockopt(mLocalSock, SOL_SOCKET, SO_SNDBUF, (const char *) &socketBuffer, sizeof(int)) == -1) {
            perror("setsocket sendbuf error");
            return false;
        }

        if (setsockopt(mLocalSock, SOL_SOCKET, SO_RCVBUF, (const char *) &socketBuffer, sizeof(int)) == -1) {
            perror("setsocket recvbuf error");
            return false;
        }

        // 启用TCP keepalive，防止空闲连接被NAT/防火墙静默丢弃
        int keepAlive = 1;
        setsockopt(mLocalSock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive));

        int keepIdle = 30;     // 空闲30秒后开始发送探测包
        int keepInterval = 5;  // 每5秒发送一次探测包
        int keepCount = 3;     // 3次无响应判定连接断开

        setsockopt(mLocalSock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle));
        setsockopt(mLocalSock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval));
        setsockopt(mLocalSock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(keepCount));
    } else {        //UdpSock没有sendbuf
        if (setsockopt(mLocalSock, SOL_SOCKET, SO_RCVBUF, (const char *) &socketBuffer, sizeof(int)) == -1) {
            perror("setsocket recvbuf error");
            return false;
        }
    }
    //To do: setUdpSocket buff
    return true;
}

//无用，全部在tcp_copy中实现
/*bool CIP_SOCKET::net_copy() {
    return false;
}*/

void CIP_SOCKET::net_close() { //安全
    if (0 != mLocalSock) {
        shutdown(mLocalSock, SHUT_RDWR);
        close(mLocalSock);
        //printf("CHANNEL %d close socket %d\n", m_dwChannel, mLocalSock);
    }

    mLocalSock = 0;
    mAcceptSock = 0;
    mLocalSock = mAcceptSock;
    mLocalAddr = mAcceptAddr;
}

bool CIP_SOCKET::net_disconnect() {
    pthread_mutex_lock(&sockMutex);
    if (0 == mAcceptSock) {
        pthread_mutex_unlock(&sockMutex);
        return false;
    }
    /*if (mAcceptSock == mLocalSock) {
        return false;
    }*/
    shutdown(mAcceptSock, SHUT_RDWR);
    close(mAcceptSock);
    //printf("[AutoTestDebug]::关闭套接字:%d\n", mAcceptSock);
    mAcceptSock = 0;
    pthread_mutex_unlock(&sockMutex);
    return true;
}

//need test
bool CIP_SOCKET::net_disconnect_udp() {
    if (0 == mLocalSock) {
        return false;
    }
    struct sockaddr_in Addr;
    Addr.sin_family = AF_UNSPEC;
    Addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    Addr.sin_port = 0;
    for (int i = 0; i <= 7; i++)
        Addr.sin_zero[i] = 0;

    if (0 > connect(mLocalSock, (struct sockaddr *) &Addr, sizeof(Addr))) {
        perror("Udp disconnect error");
    }
    return true;
}


bool CIP_SOCKET::net_refuse() {
    int ret = 0;
    if (0 == mLocalSock) {
        return false;
    }

    mAcceptSock = accept(mLocalSock, (sockaddr *) &mAcceptAddr, &mAcceptAddrLen);
    if (0 == mAcceptSock) {
        mAcceptSock = mLocalSock;
        return false;
    } else {
        shutdown(mAcceptSock, SHUT_RDWR);
        close(mAcceptSock);

        mAcceptSock = mLocalSock;
    }
    return true;
}

bool CIP_SOCKET::net_bind() {

    if (mLocalSock <= 0) {
        //printf("Error: binding an Invaild Socket\n");
        return false;
    }

    if (bind(mLocalSock, (sockaddr *) &mLocalAddr, mLocalAddrLen) < 0) {
        //perror("Bind mLocalSock");
        return false;
    }
    /*
    //利用getsockname获取端口号
	struct sockaddr_in connAddr;
	int len = sizeof(connAddr);
	ret = getsockname(m_nmLocalSocket, (SOCKADDR*)&connAddr, &len);
	m_nmLocalPort = ntohs(connAddr.sin_port);
     */
    return true;
}

bool CIP_SOCKET::net_listen(int backlog) {

    if (mLocalSock <= 0) {
        //printf("Error: Listen on Invaild Socket\n");
        return false;
    }
    int ret = listen(mLocalSock, backlog);
    if (ret == 0) {
        return true;
    }
    //printf("Error: Listen() failed\n");
    return false;
}

bool CIP_SOCKET::net_accept(int dwChannel) {
    if (mLocalSock <= 0) {
        //printf("Error: mAccept on Invaild Socket\n");
        return false;
    }

    mAcceptSock = accept(mLocalSock, (sockaddr *) &mAcceptAddr, &mAcceptAddrLen);
    if (mAcceptSock <= 0) {
        //printf("Error: mAccept() failed\n");
        return false;
    }

    // 为accepted连接也启用TCP keepalive（accept不会继承监听socket的keepalive设置）
    int keepAlive = 1;
    setsockopt(mAcceptSock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive));
    int keepIdle = 30;
    int keepInterval = 5;
    int keepCount = 3;
    setsockopt(mAcceptSock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle));
    setsockopt(mAcceptSock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval));
    setsockopt(mAcceptSock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(keepCount));

    mRemoteSock = mAcceptSock;
    mRemoteAddr = mAcceptAddr;
    mRemotePort = ntohs(mAcceptAddr.sin_port);
    mRemoteAddress = inet_ntoa(mAcceptAddr.sin_addr);
    return true;
}


unsigned long sendBytes = 0;
unsigned long sendCounts = 0;
double sendRate = 0;
double sendSpeed = 0;
unsigned long sendRuntime = 0;


DWORD CIP_SOCKET::net_Send(const std::shared_ptr<BYTE> &buf, int length) { //安全的，但应该存在SendERRNO的值不符合预期的情况
    static timeval sendStartime, sendEndtime;
    gettimeofday(&sendStartime, nullptr);
    DWORD byteTrans = 0;

    if (!buf) {
        return byteTrans;
    }
    if (mbIsMultiCast) {

    } else if (mbIsBothWay) {
        byteTrans = sendto(mAcceptSock, (char *) (buf.get()), length, 0, (sockaddr *) &mRemoteAddr,
                           sizeof(mRemoteAddr));
    } else {
        byteTrans = sendto(mAcceptSock, (char *) (buf.get()), length, 0, (sockaddr *) &mRemoteAddr,
                           sizeof(mRemoteAddr));
        //byteTrans = send(mAcceptSock, (char *) (buf.get()), length, 0);
    }

    int tempPort = ntohs(mRemoteAddr.sin_port);
    std::string tempIp= inet_ntoa(mRemoteAddr.sin_addr);

    
    //printf(">>CIPSOCKET(%d) net_send: remoteAddress:%s:%d\n", GetChannel(), tempIp.c_str(), tempPort);

    if (sendCounts >= 30000) {
        gettimeofday(&sendEndtime, nullptr);
        sendRuntime =
                (sendEndtime.tv_sec - sendStartime.tv_sec) * 1000000 + (sendEndtime.tv_usec - sendStartime.tv_usec);
        sendSpeed = static_cast<double>(sendBytes * 8) / static_cast<double>(sendRuntime + 1);
        sendBytes = 0;
        sendRate = static_cast<double>(sendCounts) * 1000000 / static_cast<double>(sendRuntime + 1);
        sendCounts = 0;
        //printf("[sendSpeedTest]: %5f Mbps, sendRate is %5f\n", sendSpeed, sendRate);
        gettimeofday(&sendStartime, nullptr);
    }
    if (byteTrans < 0) {
        SendERRNO = errno;
        /*perror("");*/
        //todo:除speedtest外缺少对发送错误和发送成功时的处理。
    }
    if (byteTrans > 0) {
        sendBytes += byteTrans;
        sendCounts++;
    }
    return byteTrans;
}

DWORD CIP_SOCKET::net_sendTo(const std::shared_ptr<BYTE> &buf, int length) {
    DWORD ret = 0;

    ret = sendto(mAcceptSock, buf.get(), length, 0,
                 (sockaddr *) &mRemoteAddr, sizeof(mRemoteAddr));
    if (0 > ret) {
        perror("sendto error");
    }
    return ret;
}

unsigned long recvBytes = 0;
unsigned long recvCounts = 0;
double recvRate = 0;
double recvSpeed = 0;
unsigned long recvRuntime = 0;
timeval recvStartime, recvEndtime;

std::shared_ptr<BYTE> CIP_SOCKET::net_Recv(unsigned long &length) {
    long ret = 0;
    if (0 <= ioctl(mAcceptSock, FIONREAD, &length)) {
        if (m_bIsUdpServer || m_bIsUdpClient) length = 1400;
        if (0 != length) {
            //TODO houlc-note : too frequently alloc memory
            //TODO houlc-todo : need to use memory pool to ease the presure from mem operation.
            std::shared_ptr<BYTE> buf(new BYTE[length], releaseArrays<BYTE>);
            mRemoteAddrLen = sizeof(mRemoteAddr);
            if (mbIsMultiCast) {
                //printf("mbIsMultiCast\n");
                return nullptr;
            } else if (0 == mRemotePort) {
                //unConnected Udp
                ret = recvfrom(mAcceptSock, (char *) (buf.get()), length, 0,
                               (struct sockaddr *) &mRemoteAddr, &mRemoteAddrLen);
                //printf("[1]Recvbytes is %d\n", ret);
                if (0 == mRemotePort) {
                    mRemoteAddress = inet_ntoa(mRemoteAddr.sin_addr);
                    mRemotePort = ntohs(mRemoteAddr.sin_port);
                    
                    printf("AVAVAVA recvfrom mRemoteIP:%s\n", mRemoteAddress.c_str());
                    printf("AVAVAVA recvfrom remotePort:%d\n", mRemotePort);

                    //20221029:发现此处存在问题，若为udp服务器则将建立其连接，只能接收一个(地址:端口)的udp套接字数据。
                    //防止UDPserver建立连接过滤数据。
                    if (!m_bIsUdpServer) {
                        connect(mAcceptSock, (struct sockaddr *) &mRemoteAddr, sizeof(mRemoteAddr));
                    }
                    //测试发现建立连接的UDP不能接收到新的广播包。
                }
                //return nullptr;

            } else if (mbIsBothWay) {
                //Connected Udp
                ret = recvfrom(mAcceptSock, (char *) (buf.get()), length, 0,
                               (struct sockaddr *) &mRemoteAddr, &mRemoteAddrLen);
                //printf("[2]Recvbytes is %d\n", ret);
            } else {
                /*ret = recv(mAcceptSock, (char *) (buf.get()), length, 0);*/
                ret = recvfrom(mAcceptSock, (char *) (buf.get()), length, 0,
                               (struct sockaddr *) &mRemoteAddr, &mRemoteAddrLen);
                //printf("[2]Recvbytes is %d\n", ret);
            }

            /*if (recvCounts >= 20000) {
                gettimeofday(&recvEndtime, NULL);
                recvRuntime =
                        (recvEndtime.tv_sec - recvStartime.tv_sec) * 1000000 +
                        (recvEndtime.tv_usec - recvStartime.tv_usec);
                recvSpeed = (double(recvBytes * 8) / recvRuntime);
                recvBytes = 0;
                recvRate = recvCounts * 1000000 / recvRuntime;
                recvCounts = 0;
                //printf("[recvSpeed Test]: %f Mbps, recvRate is %f\n", recvSpeed, recvRate);
                gettimeofday(&recvStartime, NULL);
            }*/
            /*handle error*/
            if (0 > ret) {
                RecvERRNO = errno;
                //perror("Recv");
                return nullptr;
            } else {
                //recvBytes += ret;
                //recvCounts++;
                length = ret;
                //printf("Recvbytes is %d\n", ret);
                //printf("recvCounts=%d\n", recvCounts);
                //if (length == 0)//printf("length is %d\n", length);
            }
            /*
              if (0 == m_nRemotePort)
                {
                    //如果取数据的时候发现UDP连接关闭，那么再尝试打开
                    shared_ptr<char FAR> strTemp(::inet_ntoa(m_RemoteAddr.sin_addr), releaseArraysDoNothing<char FAR>);	//静态数组，不需要程序员手动释放
                    shared_ptr<char FAR> ip = strTemp;
                    m_sRemoteAddress = ip.get();
                    m_nRemotePort = ::ntohs(m_RemoteAddr.sin_port);
                    net_connect();
                }
            */
            return buf;
        } else {
            //printf("length = 0\n");
            return nullptr;
        }

    } else {
        perror("ioctl failed");
        return nullptr;
    }
}

std::shared_ptr<BYTE> CIP_SOCKET::net_UdpRecv(unsigned long &length) { //安全的，或许存在RecvERRNO不符合预期的情况，需要注意。
    long ret = 0;
    length = UDP_PACKET_LENGTH;
    //TODO houlc-todo : same as the situation above this function.
    //                  need to use mem-pool instead of frequently operation of 'new'
    std::shared_ptr<BYTE> buf(new BYTE[length], releaseArrays<BYTE>);
    mRemoteAddrLen = sizeof(mRemoteAddr);
    if (mbIsMultiCast) {
        //printf("mbIsMultiCast\n");
        return nullptr;
    } else if (0 == mRemotePort) {
        //unConnected Udp
        ret = recvfrom(mAcceptSock, (char *) (buf.get()), length, 0,
                       (struct sockaddr *) &mRemoteAddr, &mRemoteAddrLen);
        if (0 == mRemotePort) {
            mRemoteAddress = inet_ntoa(mRemoteAddr.sin_addr);
            mRemotePort = ntohs(mRemoteAddr.sin_port);
            //20221029:发现此处存在问题，若为udp服务器则将建立其连接，只能接收一个(地址:端口)的udp套接字数据。
            //防止UDPserver建立连接过滤数据。
            if (!m_bIsUdpServer) connect(mAcceptSock, (struct sockaddr *) &mRemoteAddr, sizeof(mRemoteAddr));
            //测试发现建立连接的UDP不能接收到新的广播包。
        }
    } else if (mbIsBothWay) {
        //Connected Udp
        ret = recvfrom(mAcceptSock, (char *) (buf.get()), length, 0,
                       (struct sockaddr *) &mRemoteAddr, &mRemoteAddrLen);
    } else {
        ret = recvfrom(mAcceptSock, (char *) (buf.get()), length, 0,
                       (struct sockaddr *) &mRemoteAddr, &mRemoteAddrLen);
    }
    /*if (recvCounts >= 20000) {
        gettimeofday(&recvEndtime, NULL);
        recvRuntime =
                (recvEndtime.tv_sec - recvStartime.tv_sec) * 1000000 +
                (recvEndtime.tv_usec - recvStartime.tv_usec);
        recvSpeed = (double(recvBytes * 8) / recvRuntime);
        recvBytes = 0;
        recvRate = recvCounts * 1000000 / recvRuntime;
        recvCounts = 0;
        //printf("[recvSpeed Test]: %f Mbps, recvRate is %f\n", recvSpeed, recvRate);
        gettimeofday(&recvStartime, NULL);
    }*/
    if (0 > ret) {
        RecvERRNO = errno;
        return nullptr;
    } else {
        //recvBytes += ret;
        //recvCounts++;
        length = ret;
    }
    return buf;
}

bool CIP_SOCKET::net_connect() {
    if (mLocalSock <= 0) {
        //printf("Error: Connect on Invaild Socket\n");
        return false;
    }
    mRemoteAddrLen = sizeof(mRemoteAddr);
    if (connect(mLocalSock, (sockaddr *) &mRemoteAddr, mRemoteAddrLen) < 0) {
        //perror("connect failed");
        return false;
        //Todo: error handle
    }
    //Todo: get sockport and address information , store
    mAcceptSock = mLocalSock;
    return true;
}

//TODO: syscall param epoll_ctl() points to uninitialised byte(s)
//解决方法:初始化TempEvent
bool CIP_SOCKET::EpollEventsManager(int EpollFd, int Socket, int Operation, uint32_t Events, void *eventdataEx) {
    epoll_event TempEvent{};
    TempEvent.events = Events;
    TempEvent.data.fd = Socket;
    //TempEvent.data.ptr = eventdataEx;
    //TempEvent.data.ptr = nullptr;
    if ((Operation == EPOLL_CTL_ADD) && (Events & EPOLLET)) {
        if (setsockNoblock(Socket)) {
            //printf("Set Socket {%d} NoBlock\n", Socket);
        }
    }
    if (Operation == EPOLL_CTL_DEL) {
        //printf("Socket:%d is delete regist in EpollFd:%d , ", Socket, EpollFd);
        //delete (EventInfo *) eventdataEx;
    } else {
        //printf("Socket:%d is register in EpollFd:%d , ", Socket, EpollFd);
    }
    if (epoll_ctl(EpollFd, Operation, Socket, &TempEvent) < 0) {
        //printf("Failed\n");
        //perror("EpollEventManager Error");
        return false;
    }
    //printf("Successfully\n");
    return true;
}

bool CIP_SOCKET::setsockNoblock(int sockfd) {
    if (2 >= sockfd)return false;
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1) {
        //printf("Error: setsockNoblocking failed\n");
        return false;
    }
    return true;
}

void CIP_SOCKET::SetChannel(DWORD dwChannel) {
     m_dwChannel = dwChannel;

     //------infohub----------
     rx_cid_rate_sttable.begin(dwChannel);
     tx_cid_rate_sttable.begin(dwChannel);
}

