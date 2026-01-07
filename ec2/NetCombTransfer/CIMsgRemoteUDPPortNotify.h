//
// Created by 王炳棋 on 2022/11/15.
//

#ifndef NETCOMBTRANSFER_CIMSGREMOTEUDPPORTNOTIFY_H
#define NETCOMBTRANSFER_CIMSGREMOTEUDPPORTNOTIFY_H

#include "NetCombTransferBase.h"
#include "CIMsgChannelStatusNotify.h"

class CIMsgRemoteUDPPortNotify :
        public CIMsg {
public:
    UINT m_nUDPPort;    //UDP端口号

public:
    CIMsgRemoteUDPPortNotify() : CIMsg() {
        m_nUDPPort = 0;
    }

    virtual ~CIMsgRemoteUDPPortNotify() {
    };

private:
    const static int CIMsgRemoteUDPPortNotify_Length = 9;
    const static int CIMsgRemoteUDPPortNotify_Least_Meaning_Length = 9;

public:
    virtual DWORD GetCommandID() const override {
        return eCommandRemoteUDPPortNotify;
    }

    std::shared_ptr<CIMsg> CopyFrom() const override {
        try {
            std::shared_ptr<CIMsgRemoteUDPPortNotify> pObject = std::make_shared<CIMsgRemoteUDPPortNotify>();
            if (pObject) {
                pObject->m_dwDeviceID = m_dwDeviceID;
                pObject->m_nUDPPort = m_nUDPPort;

                return pObject;
            } else {
                return nullptr;
            }
        }
        catch (...) {
            return nullptr;
        }
    }

    std::shared_ptr<BYTE> Encode(DWORD &nLength) override {
        try {
            nLength = CIMsgRemoteUDPPortNotify_Length;
            std::shared_ptr<BYTE> pBuffer(new BYTE[nLength], releaseArrays<BYTE>);

            if (pBuffer && pBuffer.get()) {
                BYTE *pTemp = pBuffer.get();
                //命令标志字节
                pTemp[0] = 1;
                pTemp += 1;
                DWORD dw_data = GetCommandID();
                WriteData(pTemp, dw_data);
                pTemp += 4;
                WriteData(pTemp, m_nUDPPort);

                return pBuffer;
            }
            nLength = 0;
            return nullptr;
        }
        catch (...) {
            nLength = 0;
            return nullptr;
        }
    }

    bool Decode(std::shared_ptr<BYTE> pBuffer, DWORD nLength) override {
        try {
            if (pBuffer && pBuffer.get() && nLength >= CIMsgRemoteUDPPortNotify_Least_Meaning_Length) {
                DWORD dwData = 0;
                BYTE *pTemp = pBuffer.get();

                pTemp += 1;    //跳过命令标志位
                dwData = ReadData(pTemp);
                pTemp += 4;
                m_nUDPPort = ReadData(pTemp);

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
        return CIMsgRemoteUDPPortNotify_Length;
    }
};

#endif //NETCOMBTRANSFER_CIMSGREMOTEUDPPORTNOTIFY_H
