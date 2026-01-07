//
// Created by 王炳棋 on 2022/11/15.
//

#ifndef NETCOMBTRANSFER_CIMSGREMOTEUDPPORTREQUEST_H
#define NETCOMBTRANSFER_CIMSGREMOTEUDPPORTREQUEST_H

#include "NetCombTransferBase.h"
#include "CIMsgChannelStatusNotify.h"

class CIMsgRemoteUDPPortRequest :
        public CIMsg {
public:
    CIMsgRemoteUDPPortRequest() {}

    virtual ~CIMsgRemoteUDPPortRequest() {}

public:
    const static int CIMsgRemoteUDPPortRequest_Length = 5;
    const static int CIMsgRemoteUDPPortRequest_Leasr_Meaning_Length = 5;

public:
    DWORD GetCommandID() const override {
        return eCommandRemoteUDPPortRequest;
    }

    std::shared_ptr<BYTE> Encode(DWORD &nLength) override {
        try {
            nLength = CIMsgRemoteUDPPortRequest_Length;
            std::shared_ptr<BYTE> pBuffer(new BYTE[nLength], releaseArrays<BYTE>);
            if (pBuffer && pBuffer.get()) {
                BYTE *pTemp = pBuffer.get();
                pTemp[0] = 1;
                pTemp += 1;
                DWORD dwData = GetCommandID();
                WriteData(pTemp, dwData);

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
            if (pBuffer && pBuffer.get() && nLength >= CIMsgRemoteUDPPortRequest_Leasr_Meaning_Length) {
                DWORD dwData = 0;
                BYTE *pTemp = pBuffer.get();
                pTemp += 1;
                dwData = ReadData(pTemp);
                return true;
            }
            return false;
        } catch (...) {
            return false;
        }
    }

    DWORD GetLength() const override {
        return CIMsgRemoteUDPPortRequest_Length;
    }

    std::shared_ptr<CIMsg> CopyFrom() const override {
        try {
            std::shared_ptr<CIMsgRemoteUDPPortRequest> pObject = std::make_shared<CIMsgRemoteUDPPortRequest>();
            if (pObject) {
                pObject->m_dwDeviceID = m_dwDeviceID;
                return pObject;
            }
            return nullptr;
        } catch (...) {
            return nullptr;
        }
    }
};

#endif //NETCOMBTRANSFER_CIMSGREMOTEUDPPORTREQUEST_H
