//
// Created by 王炳棋 on 2022/10/4.
//

#ifndef EPOLLCOMM_CUDP_H
#define EPOLLCOMM_CUDP_H

#include "CIPSOCKET.h"
#include "./EpollCommApi.h"

#define UdpMaxChannelNum 1024
#define UDP_MAX_PACKET_NUMBER 500

class CUDP
        : public CIP_SOCKET {
public:
    CUDP();

    CUDP(int EpollFd, PDATACALLBACK *pReceiveDataCallback, bool canRead, bool canWrite);

    ~CUDP();

public:
    PDATACALLBACK *m_pReceiveDataCallback = nullptr;
    PDATACALLBACK *m_pReceiveDataForThirdPartyTransfer = nullptr;

    void SetUdpServerChannelRemoteAddress(const std::string& remote_address, DWORD remote_port);

    void Restart();

private:
    pthread_mutex_t udpMutex;
    int UdpEpollFd = 0;
    bool m_bConnected = false;//目前在析构、udp_close、EPOLLIN时改变，没发现有啥用
    bool m_bIsServer = false;//在构造和open变化
    bool m_bCanWrite = false;//
    bool m_bInRead = false;//新增加，用于判断此套接字是否正接受数据
    bool m_bCanRead = false;//
    bool m_bInit = false;//析构，open，close
    bool m_bStartRead = false;//=构造析构，EPOLLIN仅用来判断
    bool m_bIsFirstWriteMessage = false;
    bool m_bIsBothWay = false;    //如果时true，是需要与第三方接口通信的UDP

private:
    unsigned bufferIndex = 0;
    std::unique_ptr<NetBuffer[]> UdpBuffers = nullptr;
public:
    void FreeAllMemory();
    //GetBuffer is necessary?
    //NetBuffer m_BufferList[_UDP_MAX_PACKET_NUMBER_];
public:
    void udp_init(UINT nLocalPort, UINT nRemotePort, std::string szLocalAddress,
                  const std::string& szREmoteAddress);

    //组播初始化方法
    void udp_init(UINT nLocalPort, UINT nRemotePort, std::string szLocalAddress,
                  std::string szRemoteAddress, std::string remoteMultiCastIp, std::string selfMultiCastIp);

    bool udp_open(bool IsServer);

    bool udp_close();

    std::shared_ptr<BYTE> udp_read(unsigned long &length);

    bool udp_write(const std::shared_ptr<BYTE>& buf, int length);

    long OnEvent(uint32_t Events);

};


#endif //EPOLLCOMM_CUDP_H
