//
// Created by Kong on 2023/6/9.
//

#ifndef RBUDP_SERIALIZEUTIL_H
#define RBUDP_SERIALIZEUTIL_H

#endif //RBUDP_SERIALIZEUTIL_H
#include "vector"
#include "memory"
#include "CRBUDPTerm2TermConfig.h"

namespace RBUDP
{
    /**
     * 这个头文件主要是提供一些供序列和反序列化的工具函数
     */

    /**
     * 将一个32位的数按照高位高字节，低位低字节的顺序写到一个字符数组中去
     */
    void WriteData(BYTE *pBuffer, DWORD dwData);

    /**
    * 将一个16位的数按照高位高字节，低位低字节的顺序写到一个字符数组中去
    */
    //void WriteData_16(BYTE* dst, unsigned short data);

    /**
     * 把pszString中从第1到nLength字节的数据都复制到pBuffer中去
     */
    void WriteChar(BYTE* pBuffer, BYTE* pszString, DWORD nLength);

    /**
     * 从pBuffer中按照高位高字节，低位低字节的顺序读取一个32位无符号整数作为返回值
     */
    DWORD ReadData(BYTE* pBuffer);

    /**
    * 从pBuffer中按照高位高字节，低位低字节的顺序读取一个16位无符号整数作为返回值
    */
    //WORD ReadData_16(BYTE* pBuffer);

    /**
     * 从pBuffer中读取一个长度为nLength的字符序列，并在所返回的字符序列的末尾加上一个'\0'字符。
     */
    BYTE* ReadChar(BYTE* pBuffer, DWORD nLength);

    /**
     * 为RBUDP发送非可靠数据提供编码服务
     */
    std::shared_ptr<BYTE> RBUDP_Encode_Bytes_Without_Reliable(std::shared_ptr<BYTE> bytes, DWORD old_len, DWORD& new_len);
    /**
     * 解码RBUDP的不可靠数据
     */
    std::shared_ptr<BYTE> RBUDP_Decode_Data_Without_Reliable(std::shared_ptr<BYTE> bytes, DWORD &nLength);
}