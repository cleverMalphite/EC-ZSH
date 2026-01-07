//
// Created by 王炳棋 on 2022/11/16.
//

#ifndef NETCOMBTRANSFER_CIMSGREMOTEIDREQUEST_H
#define NETCOMBTRANSFER_CIMSGREMOTEIDREQUEST_H

#include "NetCombTransferBase.h"
#include "CIMsgChannelStatusNotify.h"

class CIMsgRemoteIDRequest :
        public CIMsg {
public:
    CIMsgRemoteIDRequest() {}

    virtual ~CIMsgRemoteIDRequest() {}

private:
    const static int CIMsgRemoteIDRequest_Length = 5;
    const static int CIMsgRemoteIDRequest_Least_Meaning_Length = 5;

public:
    DWORD GetCommandID() const override {
        return eCommandRemoteIDRequest;
    }

    std::shared_ptr<BYTE> Encode(DWORD &nLength) override {
        try {
            nLength = CIMsgRemoteIDRequest_Length;
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
            if (pBuffer && pBuffer.get() && nLength >= CIMsgRemoteIDRequest_Least_Meaning_Length) {
                DWORD dwData = 0;
                BYTE *pTemp = pBuffer.get();
                pTemp += 1;
                dwData = ReadData(pTemp);
                return true;
            } else {
                return false;
            }
        } catch (...) {
            return false;
        }
    }

    DWORD GetLength() const override {
        return CIMsgRemoteIDRequest_Length;
    }

    std::shared_ptr<CIMsg> CopyFrom() const override {
        std::shared_ptr<CIMsgRemoteIDRequest> pObject = std::make_shared<CIMsgRemoteIDRequest>();
        if (pObject && pObject.get()) {
            pObject->m_dwDeviceID = m_dwDeviceID;
            return pObject;
        }
        return nullptr;
    }
};

#endif //NETCOMBTRANSFER_CIMSGREMOTEIDREQUEST_H
