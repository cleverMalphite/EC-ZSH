//
// Created by 王炳棋 on 2022/12/20.
//

#include <vector>
#include <memory>
#include "../mGlobalDef.h"
#include "MRUDPTerm2TermConfig.h"

#ifndef MRUDP_SERIALIZEUTIL_H
#define MRUDP_SERIALIZEUTIL_H

namespace MRUDP {
    /**
     * 这个头文件主要是提供一些供序列和反序列化的工具函数
     */
    const static DWORD MRUDP_WITHOUT_RELIABLE_HEADER_LENGTH = 5;

    /**
     * 将一个32位的数按照高位高字节，低位低字节的顺序写到一个字符数组中去
     */
    void WriteData(BYTE *pBuffer, DWORD dwData);

    /**
     * 把pszString中从第1到nLength字节的数据都复制到pBuffer中去
     */
    void WriteChar(BYTE *pBuffer, BYTE *pszString, DWORD nLength);

    /**
     * 从pBuffer中按照高位高字节，低位低字节的顺序读取一个32位无符号整数作为返回值
     */
    DWORD ReadData(BYTE *pBuffer);

    /**
     * 从pBuffer中读取一个长度为nLength的字符序列，并在所返回的字符序列的末尾加上一个'\0'字符。
     */
    BYTE *ReadChar(BYTE *pBuffer, DWORD nLength);

    /**
     * 为MRUDP发送非可靠数据提供编码服务
     */
    std::shared_ptr<BYTE> MRUDPEncodeBytesWithoutReliable(const std::shared_ptr<BYTE>& bytes, DWORD old_len, DWORD &new_len);

    /**
     * 解码MRUDP的不可靠数据
     * nLength是解码后有效数据长度
     */
    std::shared_ptr<BYTE> MRUDPDecodeDataWithoutReliable(std::shared_ptr<BYTE> bytes, DWORD &nLength);
}

#endif //MRUDP_SERIALIZEUTIL_H
