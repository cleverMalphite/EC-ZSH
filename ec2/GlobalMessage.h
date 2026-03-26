//
// Created by 王炳棋 on 2022/11/15.
//
#include "../mGlobalDef.h"
#include <cstring>

//Todo:GoogleTest

#ifndef NETCOMBTRANSFER_GLOBALMESSAGE_H
#define NETCOMBTRANSFER_GLOBALMESSAGE_H

#define INVALID_CHANNEL_ID        0    //无效通道号
#define INVALID_TERMINAL_ID        0    //无效终端号
#define INVALID_CHANNEL_PORT    0    //无效端口号
#define GLOBAL_MAX_REGISTER_CALLBACK_FUN    10    //每个模块最多可以注册登记的回调函数
#define GLOBAL_MAX_NETWORK_CARD_NUMBER        10    //每台终端最大网络设备数量（网卡）
#define GLOBAL_MAX_TERMINAL_NUMBER            100 //网络中总的终端数量

#ifndef CHANNELNUM_RANGE
#define CHANNELNUM_RANGE
#define TCP_CHANNEL_FIRST_ID        3000    //TCP 通道分配开始号码
#define TCP_CHANNEL_FIRST_ID_MAX    3999    //TCP 通道分配最大号码
#define UDP_CHANNEL_FIRST_ID        4000    //UDP 通道分配开始号码
#define UDP_CHANNEL_FIRST_ID_MAX    4999    //UDP 通道分配最大号码
#endif

#define eModuleMessageNumber 1000

enum enumGlobalErrorCode {
    eErrorCodeOK = 0,
    eErrorCode_NetSPY = 0 - eModuleMessageNumber, //-1000:错误码范围为-1 ~ -999
    eErrorCode_NetComm = 0 - (eErrorCode_NetSPY + eModuleMessageNumber),
    eErrorCode_NetCombTransfer = 0 - (eErrorCode_NetComm + eModuleMessageNumber),
    eErrorCode_NetQosMonitor = 0 - (eErrorCode_NetCombTransfer + eModuleMessageNumber),
    eErrorCode_BigDataTransfer = 0 - (eErrorCode_NetQosMonitor + eModuleMessageNumber),
    eErrorCode_NeighborSearch = 0 - (eErrorCode_BigDataTransfer + eModuleMessageNumber),
};

enum enumGlobalMessageID {
    eMessage_Invalid = 0, //表示无效消息
    eMessage_NetSPY = eModuleMessageNumber, // NetSPY 的消息范围为 1000-1999
    eMessage_NetComm = eMessage_NetSPY + eModuleMessageNumber,
    eMessage_NetCombTransfer = eMessage_NetComm + eModuleMessageNumber,
    eMessage_NetQosMonitor = eMessage_NetCombTransfer + eModuleMessageNumber,
    eMessage_BigDataTransfer = eMessage_NetQosMonitor + eModuleMessageNumber,
    eMessage_NeighborSearch = eMessage_BigDataTransfer + eModuleMessageNumber,
    eMessage_ClientTest = eMessage_NeighborSearch + eModuleMessageNumber,
};

class CIMsg {
public:
    DWORD m_dwDeviceID;
public:
    CIMsg() {
        m_dwDeviceID = INVALID_TERMINAL_ID;
    }

    CIMsg &operator=(const CIMsg &data) {
        m_dwDeviceID = data.m_dwDeviceID;
        return *this;
    }

    void Setm_dwDeviceID(DWORD dwData) {
        m_dwDeviceID = dwData;
    }

public:
    static void WriteData(BYTE *pBuffer, DWORD dwData) {   //代码操作的是逻辑上的最高位，然后把它按大/小端序进行存放
        if (pBuffer) {   
            BYTE hhByte = (BYTE) ((dwData & 0xff000000) >> 24);
            BYTE hlByte = (BYTE) ((dwData & 0x00ff0000) >> 16);
            BYTE lhByte = (BYTE) ((dwData & 0x0000ff00) >> 8);
            BYTE llByte = (BYTE) ((dwData & 0x000000ff));
            pBuffer[0] = hhByte;
            pBuffer[1] = hlByte;
            pBuffer[2] = lhByte;
            pBuffer[3] = llByte;
        }
    }

    static void WriteData64(BYTE *pBuffer, DWORD64 dwData) {
        if (pBuffer != nullptr) {
            DWORD64 data_copy = dwData;
            DWORD highword = (DWORD) (data_copy >> 32);
            WriteData(pBuffer, highword);
            WriteData(pBuffer + 4, (DWORD) dwData);
        }
    }

    static void WriteChar(BYTE *pBuffer, BYTE *pszString, DWORD nLength) {
        if (pBuffer && pszString && nLength) {
            memcpy(pBuffer, (BYTE *) pszString, nLength);
        }
    }

    static DWORD ReadData(BYTE *pBuffer) {
        DWORD dwValue = 0;
        if (pBuffer) {
            BYTE hhByte = pBuffer[0];
            BYTE hlByte = pBuffer[1];
            BYTE lhByte = pBuffer[2];
            BYTE llByte = pBuffer[3];
            //Make Word and Make DWord
            dwValue = ((DWORD) (hhByte) << 24) + ((DWORD) hlByte << 16) + ((DWORD) lhByte << 8) + ((DWORD) llByte);
        }
        return dwValue;
    }

    static DWORD64 ReadData64(BYTE *pBuffer) {
        DWORD64 dwValue = 0;
        if (pBuffer) {
            /*DWORD HighWord = ReadData(pBuffer);
            DWORD LowWord = ReadData(pBuffer + 4);*/
            dwValue = (((DWORD64) ReadData(pBuffer)) << 32) + (DWORD64) ReadData(pBuffer + 4);
        }
        return dwValue;
    }

    static std::string ReadChar(BYTE *pBuffer, DWORD nLength) {
        std::string szValue;

        szValue.clear();
        if (pBuffer && nLength && nLength < PATH_MAX) {
            char szTemp[PATH_MAX];
            memcpy(szTemp, pBuffer, nLength);
            szTemp[nLength] = '\0';
            szValue = szTemp;
        }
        return szValue;
    }

    static DWORD DecodeCommandID(BYTE *pBuffer, DWORD nLength) {
        DWORD dwCommandID = eMessage_Invalid;
        if (pBuffer && nLength >= 1 + 4) {
            BYTE *pTemp = pBuffer;
            pTemp += 1;
            dwCommandID = ReadData(pTemp);
        }
        return dwCommandID;
    }

public:
    virtual DWORD GetCommandID() const = 0;

    virtual std::shared_ptr<CIMsg> CopyFrom() const = 0;

    virtual std::shared_ptr<BYTE> Encode(DWORD &nLength) = 0;

    virtual bool Decode(std::shared_ptr<BYTE> pBuffer, DWORD nLength) = 0;

    virtual DWORD GetLength() const = 0;

    virtual ~CIMsg() {};
};

#endif //NETCOMBTRANSFER_GLOBALMESSAGE_H
