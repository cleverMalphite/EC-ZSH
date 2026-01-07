//
// Created by 王炳棋 on 2022/10/31.
//

#ifndef EPOLLCOMM_CBCAST_H
#define EPOLLCOMM_CBCAST_H

#include "CIPSOCKET.h"
#include "./EpollCommApi.h"

#define BcastMaxChannelNum 100

class CBCast :
        public CIP_SOCKET {
public:
    int BCastEpollFd;
    std::shared_ptr<BYTE> m_pBuffer2Send;
    int m_nBufferLength = 0;
    int m_nBufferLenSended = 0;
public:
    CBCast(int EpollFd, PDATACALLBACK *pReceiveDataCallback);

    ~CBCast();

    void FreeAllMemory();

public:
    void bcast_init(UINT nBCastPort, std::string szLocalIPAddress, std::string szBCastAddr);

    bool bcast_open(bool isServer);

    bool bcast_close();

    bool bcast_write(std::shared_ptr<BYTE> buf, int Length);

    std::shared_ptr<BYTE> bcast_read(unsigned long &Length);

    long OnEvent(int SockFd, uint32_t Events);

    PDATACALLBACK *m_pReceiveDataCallback;

    int GetLocalSocket() override { return mLocalSock; }

    int GetAcceptSocket() override { return mAcceptSock; }

    bool m_bStartRead;
private:
    bool m_bConnected = false;
    bool m_bIsServer = false;
    bool m_bCanWrite = false;
    bool m_bCanRead = false;
    bool m_bInit = false;

    bool m_bReceiveChannel{};
    bool m_bIsFirstWriteMessage;

    //todo:需要一个合理的用于暂时存储数据的结构。
    //或许会需要一个锁

};

#endif //EPOLLCOMM_CBCAST_H
