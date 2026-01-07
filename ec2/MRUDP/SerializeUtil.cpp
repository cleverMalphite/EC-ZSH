//
// Created by 王炳棋 on 2023/1/1.
//
#include "SerializeUtil.h"

namespace MRUDP {
    /**
     * 将一个32位的数按照高位高字节，低位低字节的顺序写到一个字符数组中去
     */
    void WriteData(BYTE *pBuffer, DWORD dwData) {

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

    /**
     * 把pszString中从第1到nLength字节的数据都复制到pBuffer中去
     */
    void WriteChar(BYTE *pBuffer, BYTE *pszString, DWORD nLength) {

        if (pBuffer && pszString && nLength) {
            memcpy(pBuffer, (BYTE *) pszString, nLength);
        }
    }

    /**
     * 从pBuffer中按照高位高字节，低位低字节的顺序读取一个32位无符号整数作为返回值
     */
    DWORD ReadData(BYTE *pBuffer) {

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

    /**
     * 从pBuffer中读取一个长度为nLength的字符序列，并在所返回的字符序列的末尾加上一个'\0'字符。
     */
    BYTE *ReadChar(BYTE *pBuffer, DWORD nLength) {

        if (pBuffer && nLength) {
            BYTE *szValue = new BYTE[nLength];
            memcpy(szValue, (char *) pBuffer, nLength);
            return szValue;
        }

        return nullptr;
    }

    /**
     * 为MRUDP发送非可靠数据提供编码服务
     */
    std::shared_ptr<BYTE> MRUDPEncodeBytesWithoutReliable(const std::shared_ptr<BYTE>& bytes, DWORD old_len, DWORD &new_len) {
        new_len = old_len + MRUDP_WITHOUT_RELIABLE_HEADER_LENGTH;
        std::shared_ptr<BYTE> pBytes(new BYTE[new_len], releaseArrays<BYTE>);
        BYTE *pTemp = pBytes.get();
        if (nullptr != pTemp) {
            pTemp[0] = MRUDP_DATA_FIRST_BYTE_FLAG_WITHOUT_RELIABLE;
            pTemp += 1;
            WriteData(pTemp, old_len);
            pTemp += 4;
            WriteChar(pTemp, bytes.get(), old_len);
        }
        return pBytes;
    }

    /**
     * 解码MRUDP的不可靠数据
     * nLength是解码后有效数据长度
     */
    std::shared_ptr<BYTE> MRUDPDecodeDataWithoutReliable(std::shared_ptr<BYTE> bytes, DWORD &nLength) {

        BYTE *pTemp = bytes.get();
        if (nullptr != pTemp) {
            pTemp += 1;
            nLength = ReadData(pTemp);
            pTemp += 4;
        }
        std::shared_ptr<BYTE> pBytes(new BYTE[nLength], releaseArrays<BYTE>);
        WriteChar(pBytes.get(), pTemp, nLength);
        return pBytes;
    }
}
