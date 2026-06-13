//
// Created by wangbingqi on 22-9-28.
//
#include "../mGlobalDef.h"

#ifndef RESERVE_CHANNEL_BEGIN_END
#define RESERVE_CHANNEL_BEGIN_END

#define MAXCHANNELNUM 10000
//保留的客户端通道号，专供ThirdPartyTransfer之类的外部模块使用
#define RESERVE_CHANNEL_BEGIN 6500
#define RESERVE_CHANNEL_END   6599
#endif

# define UDP_PACKET_LENGTH 1400
# define GLOBAL_MAX_REGISTER_CALLBACK_FUN 10

#ifndef NETCOMBTRANSFER_MGLOBALDEF_H
#define NETCOMBTRANSFER_MGLOBALDEF_H

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned long DWORD64;
typedef unsigned int UINT;

template<typename T>
void releaseArraysDoNothing(T const *p) {

}

//定义一个数组的内存释放函数模板，用来辅助shared_ptr
template<typename T>
void releaseArrays(T *p) {
    if (__null == p || nullptr == p) {
        return;
    }
    delete[]    p;
    p = __null;
}

#endif

#ifndef EPOLLCOMM_CIPSOCKET_H
#define EPOLLCOMM_CIPSOCKET_H


#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <csignal>
#include <sys/types.h>
#include  <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <mutex>
#include <memory>

extern bool Teststart;
extern unsigned long recvBytes;
extern unsigned long recvCounts;
/*extern unsigned long sendBytes;
extern unsigned long sendCounts;*/

using std::mutex;

typedef enum {
    InvalidChannel = 0,
    TCPChannel = 1,         //tcp链路
    UDPChannel = 2,         //普通udp链路(天基终端之间的udp链路)
    UDPBothWayChannel = 3,  //udp双向链路(天基与地面站、大数据中心之间的链路)
    CBCastChannel = 4,      //udp广播链路
    MultiCastChannel = 5,   //组播(特殊的udp)链路
} enumChannelType;

class CIP_SOCKET {
public:
    CIP_SOCKET();

    ~CIP_SOCKET();

public://部分设置成private比较好，有时间优化一下。
    sockaddr_in mRemoteAddr{}, mLocalAddr{}, mAcceptAddr{};   //远程地址、本地地址、接收地址
    socklen_t mRemoteAddrLen{}, mLocalAddrLen{}, mAcceptAddrLen{};  //远程地址长度、本地地址长度、接收地址长度
    DWORD mRemotePort{}, mLocalPort{}, mAcceptPort{};
    std::string mRemoteAddress, mLocalAddress, mAcceptAddress;
    int mLocalSock = 0;
    int mRemoteSock = 0;
    int mAcceptSock = 0;
    bool m_bIsUdpClient = false;
    bool m_bIsUdpServer = false;
    DWORD m_dwChannel = 0;
public:
    bool mbIsMultiCast = false;
public:
    DWORD GetChannel() const {
        return m_dwChannel;
    }

    //创建一个新的通道的时候（TCP, UDP），会调用这个函数
    void SetChannel(DWORD dwChannel) ;

public:
    bool setsockNoblock(int sockfd);

    UINT SetLocalPort(UINT Port);

    UINT SetRemotePort(UINT Port);

    DWORD GetLocalPort();

    std::string GetLocalAddress();

    std::string GetRemoteAddress();

    std::string SetLocalAddress(std::string Address);

    std::string SetRemoteAddress(std::string Address);

protected:
    pthread_mutex_t sockMutex{};
public:
    bool net_open(int type);

    void net_close();

    bool net_copy();

    bool net_bind();

    bool net_listen(int backlog = 1);

    bool net_accept(int dwChannel = 0);

    bool net_connect();

    bool net_refuse();

    bool net_disconnect_udp();

    bool net_disconnect();

    bool EpollEventsManager(int EpollFd, int Socket, int Operation, uint32_t Events, void *Userdata);

    DWORD net_Send(const std::shared_ptr<BYTE>& buf, int length);

    std::shared_ptr<BYTE> net_Recv(unsigned long &length);

    std::shared_ptr<BYTE> net_UdpRecv(unsigned long &length);

    DWORD net_sendTo(const std::shared_ptr<BYTE>& buf, int length);

    int net_sendForBcast    ();

    //int net_recvFrom();

    //Todo : getxxxPort() and getxxxSock() and getxxxAddress()

    virtual int GetLocalSocket() { return mLocalSock; }

    virtual int GetAcceptSocket() { return mAcceptSock; }

protected:
    UINT RecvERRNO = 0;
    UINT SendERRNO = 0;

private:
    bool mbIsBothWay = false;

private:
    void SetBothWay(bool way) { mbIsBothWay = way; }
};


/*typedef struct EventInfo {
public:
    void *Ptr;
    int SockFd;
    //uint32_t Events;
public:
    EventInfo() {
        //printf("New Event, you should delete it before return\n");
        Ptr = nullptr;
        SockFd = 0;
        //Events = 0;
    }

    ~EventInfo() {
        Ptr = nullptr;
        SockFd = 0;
        //Events = 0;
    }
} EventDataEx;*/

typedef struct ip_Data_Buffer {  // 用于网络传输的缓冲区
    UINT uSocket;
    std::shared_ptr<BYTE> pBuffer;
    unsigned long dwLength = 0;

    ip_Data_Buffer() {
        uSocket = 0;
        pBuffer = nullptr;
        dwLength = 0;
    }

    ~ip_Data_Buffer() {
        uSocket = 0;
        pBuffer = nullptr;
        dwLength = 0;
    }
} NetBuffer;

#endif //EPOLLCOMM_CIPSOCKET_H
