//
// Created by 王炳棋 on 2022/9/29.
//

#ifndef EPOLLCOMM_CTCP_H
#define EPOLLCOMM_CTCP_H

#include "CIPSOCKET.h"
#include "MCPtrList.h"
#include "./EpollCommApi.h"

#define TcpMaxChannelNum 100

class CTCP
        : public CIP_SOCKET {
public:
    CTCP();

    CTCP(int TcpEpollFd, PDATACALLBACK *m_pReceiveDataCallback);

    ~CTCP();

public:

    DWORD GetNextChannel();

    void SetChannel_TcpList(DWORD ChannelHead, DWORD ChannelNumber);

    PDATACALLBACK *m_pReceiveDataCallback = nullptr;

public:
    void FreeAllMemory();

    //std::shared_ptr<BYTE> GetBuffer(unsigned long &length);

public:

    bool tcp_isMCU() const;

    bool isAllChannelListenOver() const;

    inline bool tcp_IsServer() {
        return m_bIsServer;
    }

    void tcp_init(UINT localPort, UINT remotePort, std::string localAddress, std::string remoteAddress);

    bool tcp_open(bool IsServer);

    bool tcp_newCopy(const std::shared_ptr<CTCP>& TcpServer, DWORD dwChannel);

    bool tcp_close();

    bool tcp_write(const std::shared_ptr<BYTE>& buf, int length);

    bool TcpEpollWrite();

    std::shared_ptr<BYTE> tcp_read(unsigned long &length);

    long OnEvent(int SockFd, uint32_t Events);

    long NewOnEvent(uint32_t Events);

    void SetIsDTUChannel(bool isDTUChannel = false);

    bool GetIsDTUChannel();

public:
    std::shared_ptr<BYTE> m_pBuffer2Send = nullptr;
    int m_nBufferLength = 0;    //记录要发送序列的总长度
    int m_nBufferLenSended = 0;    //记录已经发送到了哪个字节
    bool m_bReadyForKeepAliveCheck = false;    //为true表示此链路可以接受心跳检测了

    int EpollFd = 0;
    MCPtrList<NODE> mChannelList;
    DWORD m_dwChannelNumber = TcpMaxChannelNum;
    DWORD m_dwChannelCurrentNumber = 0;

    bool m_IsDTUChannel = false;

private:
    /*std::mutex tcpMutex;*/
    pthread_mutex_t tcpmtx;
    bool m_bConnected = false;
    bool m_bIsServer = false;
    bool m_bCanWrite = false;
    bool m_bInRead = false;
    bool m_bOpenReady = false;
    bool m_bCanRead = false;
    bool m_nChannelType = TCPChannel;
    bool m_bReceiveChannel = false;
    bool m_bIsFirstWriteMessage = true;

    //tcp net info for infohub
    uint32_t m_rtt = 0;
    uint32_t m_loss = 0;

};


#endif //EPOLLCOMM_CTCP_H
