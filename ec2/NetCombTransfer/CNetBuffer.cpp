//
// Created by 王炳棋 on 2022/11/14.
//
#include "CNetBuffer.h"

#include <utility>

CNetBuffer::CNetBuffer() {

}

CNetBuffer::CNetBuffer(UINT uSocket, std::shared_ptr<BYTE> pBuffer, unsigned long dwLength,
                       int64_t data_recv_timeStamp) :
        m_uSocket(uSocket), m_pBuffer(std::move(pBuffer)),
        m_dwLength(dwLength), m_data_recv_timeStamp(data_recv_timeStamp) {
    //或许pBuffer需要深拷贝
}

CNetBuffer::~CNetBuffer() {
    m_uSocket = 0;
    m_pBuffer = nullptr;
    m_dwLength = 0;
    m_data_recv_timeStamp = 0;
}