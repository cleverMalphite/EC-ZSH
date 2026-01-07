//
// Created by 王炳棋 on 2022/11/16.
//

#ifndef NETCOMBTRANSFER_CIMSGROUTERINFONOTIFY_H
#define NETCOMBTRANSFER_CIMSGROUTERINFONOTIFY_H

//#include "NetCommApi.h"
#include "NetCombTransferBase.h"
#include "CIMsgRouterInfoDeleteNotify.h"

class CIMsgRouterInfoNotify :
        public CIMsg {

public:
    DWORD m_dwDestinationTID;    //目的端TID
    DWORD m_dwSourceTID;    //源端TID
    UINT m_nNop;    //跳数
    UINT m_nQosSend;    //前向链路，即m_dwSourceTID端向m_dwDestinationTID端发送数据的前向链路对应的QoS反馈值
    UINT m_nQosRecv;    //返向链路，即m_dwDestinationTID端向m_dwSourceTID端发送数据的返向链路对应的QoS反馈值

    CIMsgRouterInfoNotify() :
            m_dwDestinationTID(0), m_dwSourceTID(0),
            m_nNop(0), m_nQosRecv(0), m_nQosSend(0) {
    }

    virtual ~CIMsgRouterInfoNotify() {}

private:
    const static int CIMsgRouterInfoNotify_Length = 25;
    const static int CIMsgRouterInfoNotify_Least_Meaning_Length = 25;


public:
    DWORD GetCommandID() const override {
        return eCommandRouterInfoNotify;
    }

    std::shared_ptr<BYTE> Encode(DWORD &nLength) override {
        try {
            nLength = CIMsgRouterInfoNotify_Length;
            std::shared_ptr<BYTE> pBuffer(new BYTE[CIMsgRouterInfoNotify_Length], releaseArrays<BYTE>);
            if (pBuffer && pBuffer.get()) {
                BYTE *pTemp = pBuffer.get();
                //标志位置1，表示是指令
                pTemp[0] = 1;
                pTemp += 1;//写数据类别
                DWORD dw_data = GetCommandID();
                WriteData(pTemp, dw_data);
                pTemp += 4;
                WriteData(pTemp, m_dwDestinationTID);
                pTemp += 4;
                WriteData(pTemp, m_dwSourceTID);
                pTemp += 4;
                WriteData(pTemp, m_nNop);
                pTemp += 4;
                WriteData(pTemp, m_nQosSend);
                pTemp += 4;
                WriteData(pTemp, m_nQosRecv);

                return pBuffer;
            } else {
                nLength = 0;
                return nullptr;
            }
        }
        catch (...) {
            nLength = 0;
            return nullptr;
        }
    }

    bool Decode(std::shared_ptr<BYTE> pBuffer, DWORD nLength) override {
        try {
            if (pBuffer && pBuffer.get() && nLength >= CIMsgRouterInfoNotify_Least_Meaning_Length) {
                DWORD dwData = 0;
                BYTE *pTemp = pBuffer.get();
                pTemp += 1;
                dwData = ReadData(pTemp);
                pTemp += 4;
                //注意这里反序列化的顺序要与序列化的顺序保持一致
                m_dwDestinationTID = ReadData(pTemp);
                pTemp += 4;
                m_dwSourceTID = ReadData(pTemp);
                pTemp += 4;
                m_nNop = ReadData(pTemp);
                pTemp += 4;
                m_nQosSend = ReadData(pTemp);
                pTemp += 4;
                m_nQosRecv = ReadData(pTemp);

                return true;
            } else {
                return false;
            }
        }
        catch (...) {
            return false;
        }
    }

    DWORD GetLength() const override {
        return CIMsgRouterInfoNotify_Length;
    }

    std::shared_ptr<CIMsg> CopyFrom() const override {
        try {
            std::shared_ptr<CIMsgRouterInfoNotify> pObject = std::make_shared<CIMsgRouterInfoNotify>();
            if (pObject) {
                pObject->m_dwDeviceID = m_dwDeviceID;
                pObject->m_dwDestinationTID = m_dwDestinationTID;
                pObject->m_dwSourceTID = m_dwSourceTID;
                pObject->m_nNop = m_nNop;
                pObject->m_nQosSend = m_nQosSend;
                pObject->m_nQosRecv = m_nQosRecv;
                return pObject;
            } else {
                return nullptr;
            }
        }
        catch (...) {
            return nullptr;
        }
    }
};

#endif //NETCOMBTRANSFER_CIMSGROUTERINFONOTIFY_H
