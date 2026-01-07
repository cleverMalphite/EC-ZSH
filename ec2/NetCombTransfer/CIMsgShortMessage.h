//
// Created by 王炳棋 on 2022/11/16.
//

#ifndef NETCOMBTRANSFER_CIMSGSHORTMESSAGE_H
#define NETCOMBTRANSFER_CIMSGSHORTMESSAGE_H

//#include "CIP_SOCKET.h"
#include <utility>

#include "NetCombTransferBase.h"
#include "CIMsgRouterInfoDeleteNotify.h"

class CIMsgShortMessage
        : public CIMsg {
public:
    CIMsgShortMessage() {}

    CIMsgShortMessage(DWORD send_tid, DWORD recv_tid,
                      DWORD data_length, std::shared_ptr<BYTE> p_data) :
            m_send_tid(send_tid), m_recv_tid(recv_tid),
            m_data_length(data_length), m_p_data(std::move(p_data)) {
    }

    virtual ~CIMsgShortMessage() {}

public:
    DWORD GetCommandID() const override {
        return eCommandShortMessage;
    }

    DWORD GetLength() const override {
        return m_data_length;
    }

    std::shared_ptr<BYTE> Encode(DWORD &nLength) override {
        try {
            nLength = m_data_length + DATA_HEADER_LENGTH;
            std::shared_ptr<BYTE> pBuffer(new BYTE[nLength], releaseArrays<BYTE>);
            if (pBuffer && pBuffer.get()) {
                BYTE *pTemp = pBuffer.get();
                pTemp[0] = 1;
                pTemp += 1;
                DWORD dwData = GetCommandID();
                WriteData(pTemp, dwData);
                pTemp += 4;
                WriteData(pTemp, m_send_tid);
                pTemp += 4;
                WriteData(pTemp, m_recv_tid);
                pTemp += 4;
                WriteData(pTemp, m_data_length);
                pTemp += 4;
                WriteChar(pTemp, m_p_data.get(), m_data_length);
                return pBuffer;
            } else {
                nLength = 0;
                return nullptr;
            }
        } catch (...) {
            nLength = 0;
            return nullptr;
        }
    }

    bool Decode(std::shared_ptr<BYTE> pBuffer, DWORD nLength) override {
        try {
            if (pBuffer && pBuffer.get() && nLength >= DATA_HEADER_LENGTH) {
                m_p_total_data = pBuffer;
                BYTE *pTemp = pBuffer.get();
                timeval timeStamp;
                gettimeofday(&timeStamp, nullptr);
                pTemp += 1;
                DWORD dwCommand = ReadData(pTemp);
                pTemp += 4;
                m_send_tid = ReadData(pTemp);
                pTemp += 4;
                m_recv_tid = ReadData(pTemp);
                pTemp += 4;
                m_data_length = ReadData(pTemp);
                pTemp += 4;
                m_total_length = m_data_length + DATA_HEADER_LENGTH;
                packet_decode_TimeStamp = (timeStamp.tv_sec * 1000000 + timeStamp.tv_usec) / 1000;
                //The time is expressed in seconds and microseconds since midnight (0 hour), January 1, 1970.
                if (m_total_length <= nLength) {
                    std::shared_ptr<BYTE> pData(new BYTE[m_data_length], releaseArrays<BYTE>);
                    //memcpy的第三个参数是字节数，且要保证其不大于dst的大小。
                    memcpy(pData.get(), pTemp, m_data_length);//win平台此处使用了内存安全的memcpy_s函数，这在linux上没有实现。
                    m_p_data = pData;
                    if (m_total_length != 65) {
                        //printf("NetComb data decode wrong, expect length 64 while true length {%d}\n", m_total_length);
                    }
                    return true;
                } else {
                    return false;
                }
            }
            return false;
        } catch (...) {
            return false;
        }
    }

    std::shared_ptr<CIMsg> CopyFrom() const override {
        std::shared_ptr<CIMsgShortMessage> pObject = std::make_shared<CIMsgShortMessage>();
        if (pObject && pObject.get()) {
            pObject->m_send_tid = m_send_tid;
            pObject->m_recv_tid = m_recv_tid;
            pObject->m_dwDeviceID = m_dwDeviceID;
            pObject->m_p_data = m_p_data;
            pObject->m_data_length = m_data_length;
            pObject->m_total_length = m_total_length;
            pObject->m_p_total_data = m_p_total_data;
            pObject->packet_decode_TimeStamp = packet_decode_TimeStamp;
            return pObject;
        }
        return nullptr;
    }

public:
    DWORD GetSendTID() const { return m_send_tid; }

    DWORD GetRecvTID() const { return m_recv_tid; }

    DWORD GetTotalLength() const { return m_total_length; }

    std::shared_ptr<BYTE> GetDataBuffer() { return m_p_data; }

    std::shared_ptr<BYTE> GetTotalData() { return m_p_total_data; }

    int64_t Getpacket_decode_TimeStamp() const { return packet_decode_TimeStamp; }

private:
    DWORD m_data_length;
    DWORD m_send_tid;
    DWORD m_recv_tid;
    DWORD m_total_length; //数据未被解析时的总长度
    std::shared_ptr<BYTE> m_p_total_data; //未被解析时的数据
    int64_t packet_decode_TimeStamp;
    std::shared_ptr<BYTE> m_p_data;
public:
    const static DWORD DATA_HEADER_LENGTH = 1 + 4 + 4 + 4 + 4;
    //标识（1字节）+命令标识（4字节）+发送端TID（4字节）+接收端（4字节）+ 数据长度（4字节）
    const static DWORD DATA_HEADER_LENGTH_OFFSET = 1 + 4 + 4 + 4;
};


#endif //NETCOMBTRANSFER_CIMSGSHORTMESSAGE_H
