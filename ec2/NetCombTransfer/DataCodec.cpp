//
// Created by 王炳棋 on 2022/11/17.
//

#include "DataCodec.h"
//#include "NetCommApi.h"

//使用了较多的memcpy，memcpy是危险的函数，调试时应当关注。

DataCodec::DataCodec() = default;

DataCodec::~DataCodec() = default;

bool DataCodec::IsMessage(const std::shared_ptr<BYTE>& pBuffer, DWORD nLength) {
    try {
        if (pBuffer && pBuffer.get() && nLength) {
            int nMsg = pBuffer.get()[0];
            if (nMsg) {
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    } catch (...) {
        return false;
    }
}

void DataCodec::WriteData(BYTE *pBuffer, DWORD dwData) {
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

DWORD DataCodec::ReadData(const BYTE *pBuffer) {
    DWORD dwValue = 0;
    if (pBuffer) {
        BYTE hhByte = pBuffer[0];
        BYTE hlByte = pBuffer[1];
        BYTE lhByte = pBuffer[2];
        BYTE llByte = pBuffer[3];
        dwValue = ((DWORD) (hhByte) << 24) + ((DWORD) hlByte << 16) + ((DWORD) lhByte << 8) + ((DWORD) llByte);
    }
    return dwValue;
}

bool DataCodec::GetRouterInfo(const std::shared_ptr<BYTE>& pBuffer, DWORD nLength, DWORD &dwSourceTID, DWORD &dwDestinationTID) {
    /*
     * 路由信息的消息格式
     * 标志（1字节）+源端（4字节）+目的端TID（4字节）
     */
    try {
        if (pBuffer && pBuffer.get() && nLength > 0) {
            BYTE *pTemp = pBuffer.get();
            pTemp += 1;
            dwSourceTID = ReadData(pTemp);
            pTemp += 4;
            dwDestinationTID = ReadData(pTemp);
            return true;
        } else {
            return false;
        }
    } catch (...) {
        return false;
    }
}

//1字节 标志
//4字节 源端TID
//4字节 目的端TID

//nLength是数据部分的长度
std::shared_ptr<BYTE>
DataCodec::EncodeData(const std::shared_ptr<BYTE>& pBuffer, DWORD nLength, DWORD dwSourceTID, DWORD dwDestinationTID,
                      DWORD &nOutLength) {
    try {
        nOutLength = 0;
        if (pBuffer && pBuffer.get() && nLength) {
            nOutLength = nLength + 1 + 4 + 4;
            std::shared_ptr<BYTE> pEncodeBuffer(new BYTE[nOutLength], releaseArrays<BYTE>);
            if (pEncodeBuffer && pEncodeBuffer.get()) {
                BYTE *pTemp = pEncodeBuffer.get();
                pTemp[0] = 0;
                pTemp += 1;
                WriteData(pTemp, dwSourceTID);
                pTemp += 4;
                WriteData(pTemp, dwDestinationTID);
                pTemp += 4;
                memcpy(pTemp, pBuffer.get(), nLength);
                return pEncodeBuffer;
            } else {
//                printf("不应该发生这个错误叭\n");
                nOutLength = 0;
                return nullptr;
            }
        } else {
//            printf("nLength = %d\n", nLength);
            return nullptr;
        }
    } catch (...) {
        nOutLength = 0;
        return nullptr;
    }
}

//nLength是整段需要Decode的长度：头部+数据长度
std::shared_ptr<BYTE>
DataCodec::DecodeData(const std::shared_ptr<BYTE>& pBuffer, DWORD nLength, DWORD &dwSourceTID, DWORD &dwDestinationTID,
                      DWORD &nOutLength) {
    try {
        nOutLength = 0;
        //TODO:这里的4+4+1将来要用一个消息类里面的宏代替
        if (pBuffer && pBuffer.get() && nLength > (4 + 4 + 1)) {
            BYTE *pTemp = pBuffer.get();
            pTemp += 1;
            dwSourceTID = ReadData(pTemp);
            pTemp += 4;
            dwDestinationTID = ReadData(pTemp);
            pTemp += 4;
            nOutLength = nLength - (1 + 4 * 2);
            std::shared_ptr<BYTE> pDecodeBuffer(new BYTE[nOutLength], releaseArrays<BYTE>);
            if (pDecodeBuffer && pDecodeBuffer.get()) {
                memcpy(pDecodeBuffer.get(), pTemp, nOutLength);
                return pDecodeBuffer;
            } else {
                nOutLength = 0;
                return nullptr;
            }
        } else {
            return nullptr;
        }
    } catch (...) {
        nOutLength = 0;
        return nullptr;
    }
}

std::shared_ptr<BYTE>
DataCodec::EncodeMsg(const std::shared_ptr<BYTE>& pBuffer, DWORD nLength, DWORD dwSourceTID, DWORD dwDestinationTID,
                     DWORD &nOutLength) {
    try {
        nOutLength = 0;
        if (pBuffer && pBuffer.get() && nLength) {
            nOutLength = nLength + 1 + 4 * 2;
            std::shared_ptr<BYTE> pEncodeBuffer(new BYTE[nOutLength], releaseArrays<BYTE>);
            if (pEncodeBuffer && pEncodeBuffer.get()) {
                BYTE *pTemp = pEncodeBuffer.get();
                pTemp[0] = 1;
                pTemp += 1;
                WriteData(pTemp, dwSourceTID);
                pTemp += 4;
                WriteData(pTemp, dwDestinationTID);
                pTemp += 4;
                memcpy(pTemp, pBuffer.get(), nLength);

                return pEncodeBuffer;
            } else {
                nOutLength = 0;
                return nullptr;
            }
        } else {
            return nullptr;
        }
    } catch (...) {
        nOutLength = 0;
        return nullptr;
    }
}

std::shared_ptr<BYTE>
DataCodec::DecodeMsg(const std::shared_ptr<BYTE>& pBuffer, DWORD nLength, DWORD &dwSourceTID, DWORD &dwDestinationTID,
                     DWORD &nOutLength) {
    try {
        nOutLength = 0;
        if (pBuffer && pBuffer.get() && nLength > (4 * 2 + 1)) {
            BYTE *pTemp = pBuffer.get();
            pTemp += 1;
            dwSourceTID = ReadData(pTemp);
            pTemp += 4;
            dwDestinationTID = ReadData(pTemp);
            pTemp += 4;
            nOutLength = nLength - (1 + 4 * 2);
            std::shared_ptr<BYTE> pDecodeBuffer(new BYTE[nOutLength], releaseArrays<BYTE>);
            if (pDecodeBuffer && pDecodeBuffer.get()) {
                memcpy(pDecodeBuffer.get(), pTemp, nOutLength);
                return pDecodeBuffer;
            } else {
                nOutLength = 0;
                return nullptr;
            }
        } else {
            return nullptr;
        }
    } catch (...) {
        nOutLength = 0;
        return nullptr;
    }
}