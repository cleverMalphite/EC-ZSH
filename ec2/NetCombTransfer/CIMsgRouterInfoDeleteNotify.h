//
// Created by 王炳棋 on 2022/11/16.
//

#ifndef NETCOMBTRANSFER_CIMSGROUTERINFODELETENOTIFY_H
#define NETCOMBTRANSFER_CIMSGROUTERINFODELETENOTIFY_H

//#include "NetCommApi.h"

#include "NetCombTransferBase.h"
#include "CIMsgRemoteUDPPortRequest.h"

class CIMsgRouterInfoDeleteNotify :
        public CIMsg {
public:
    DWORD m_dwDestinationTID;
    DWORD m_dwSourceTID;
public:
    CIMsgRouterInfoDeleteNotify() :
            m_dwSourceTID(0),
            m_dwDestinationTID(0) {
    }

    virtual ~CIMsgRouterInfoDeleteNotify() {}

private:
    const static int CIMsgRouterInfoDeleteNotify_Length = 13;
    const static int CIMsgRouterInfoDeleteNotify_Least_Meaning_Length = 13;

public:
    DWORD GetCommandID() const override {
        return eCommandRouterInfoDeleteNotify;
    }

    std::shared_ptr<BYTE> Encode(DWORD &nLength) override {
        try {
            nLength = CIMsgRouterInfoDeleteNotify_Length;
            std::shared_ptr<BYTE> pBuffer(new BYTE[nLength], releaseArrays<BYTE>);
            if (pBuffer && pBuffer.get()) {
                BYTE *pTemp = pBuffer.get();
                pTemp[0] = 1;
                pTemp += 1;
                DWORD dwData = GetCommandID();
                WriteData(pTemp, dwData);
                pTemp += 4;
                WriteData(pTemp, m_dwDestinationTID);
                pTemp += 4;
                WriteData(pTemp, m_dwSourceTID);

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
            if (pBuffer && pBuffer.get() && nLength >= CIMsgRouterInfoDeleteNotify_Least_Meaning_Length) {
                DWORD dwData = 0;
                BYTE *pTemp = pBuffer.get();
                pTemp += 1;
                dwData = ReadData(pTemp);
                pTemp += 4;
                m_dwDestinationTID = ReadData(pTemp);
                pTemp += 4;
                m_dwSourceTID = ReadData(pTemp);

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
        return CIMsgRouterInfoDeleteNotify_Length;
    }

    std::shared_ptr<CIMsg> CopyFrom() const override {
        std::shared_ptr<CIMsgRouterInfoDeleteNotify> pObject = std::make_shared<CIMsgRouterInfoDeleteNotify>();
        if (pObject && pObject.get()) {
            pObject->m_dwDeviceID = m_dwDeviceID;
            pObject->m_dwDestinationTID = m_dwDestinationTID;
            pObject->m_dwSourceTID = m_dwSourceTID;
            return pObject;
        }
        return nullptr;
    }
};

#endif //NETCOMBTRANSFER_CIMSGROUTERINFODELETENOTIFY_H
