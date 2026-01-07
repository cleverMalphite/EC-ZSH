//
// Created by 王炳棋 on 2022/11/16.
//

#ifndef NETCOMBTRANSFER_CIMSGREMOTEIDNOTIFY_H
#define NETCOMBTRANSFER_CIMSGREMOTEIDNOTIFY_H

#include "NetCombTransferBase.h"
#include "CIMsgChannelStatusNotify.h"

/*
 * 为RIDRequest消息的类定义，主要用于序列化和非序列化RIDRequest消息
 */
class CIMsgRemoteIDNotify :
        public CIMsg {

public:
    DWORD m_dwTID;    //TID标识

private:
    const static int CIMsgRemoteIDNotify_Length = 9;
    const static int CIMsgRemoteIDNotify_Least_Meaning_Length = 5;

public:
    CIMsgRemoteIDNotify() : CIMsg() {
        m_dwTID = 0;
    };

    virtual ~CIMsgRemoteIDNotify() {
        //类内数据成员都是内置类型，没有必要手动析构
    };

public:
    DWORD GetCommandID() const override {
        return eCommandRemoteIDNotify;
    }

    //用于序列化消息，把一个CIMsgRIDNotify类的实例对象转换为字节序列
    std::shared_ptr<BYTE> Encode(DWORD &nLength) override {
        nLength = CIMsgRemoteIDNotify_Length;
        try {
            std::shared_ptr<BYTE> pBuffer(new BYTE[CIMsgRemoteIDNotify_Length], releaseArrays<BYTE>);
            if (pBuffer && pBuffer.get()) {
                BYTE *pTemp = pBuffer.get();
                //数据标志，表示这条信息是命令
                pTemp[0] = 1;
                pTemp += 1;
                DWORD dwData = GetCommandID();
                WriteData(pTemp, dwData);
                pTemp += 4;
                WriteData(pTemp, m_dwTID);
                return pBuffer;
            }
        }
        catch (...) {
            //异常退出时别忘了nLength要设置为0
            nLength = 0;
            return nullptr;
        }

        nLength = 0;
        return nullptr;
    }

    //用于反序列化消息，把字节序列转换为一个CIMsgRIDNotify类的实例对象
    bool Decode(std::shared_ptr<BYTE> pBuffer, DWORD nLength) override {
        try {
            if (pBuffer && nLength >= CIMsgRemoteIDNotify_Least_Meaning_Length) {
                DWORD dwData = 0;
                BYTE *pTemp = pBuffer.get();
                pTemp += 1;
                dwData = ReadData(pTemp);
                pTemp += 4;
                m_dwTID = ReadData(pTemp);
                return true;
            }
        }
        catch (const std::exception &) {
            return false;
        }
        return false;
    }

    DWORD GetLength() const override {
        return CIMsgRemoteIDNotify_Length;
    }

    std::shared_ptr<CIMsg> CopyFrom() const override {
        std::shared_ptr<CIMsgRemoteIDNotify> pObject = std::make_shared<CIMsgRemoteIDNotify>();
        if (pObject) {
            pObject->m_dwDeviceID = m_dwDeviceID;
            pObject->m_dwTID = m_dwTID;
            return pObject;
        }

        return nullptr;
    };
};

#endif //NETCOMBTRANSFER_CIMSGREMOTEIDNOTIFY_H
