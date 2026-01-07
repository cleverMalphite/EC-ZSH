//
// Created by 王炳棋 on 2022/11/17.
//

#ifndef NETCOMBTRANSFER_DATACODEC_H
#define NETCOMBTRANSFER_DATACODEC_H

#include "../mGlobalDef.h"

/*
 * 这个类是消息编码解码类，主要对于消息头进行编解码，具体的消息体的编解码工作由
 * 各个消息数据格式类自己负责
 */
class DataCodec {
public:
    DataCodec();

    virtual ~DataCodec();

public:
    static bool IsMessage(const std::shared_ptr<BYTE>& pBuffer, DWORD nLength);

    static void WriteData(BYTE *pBuffer, DWORD dwData);

    static DWORD ReadData(const BYTE *pBuffer);

    static std::shared_ptr<BYTE> EncodeData(const std::shared_ptr<BYTE>& pBuffer, DWORD nLength,
                                            DWORD dwSourceTID, DWORD dwDestinationTID, DWORD &nOutLength);

    static std::shared_ptr<BYTE> DecodeData(const std::shared_ptr<BYTE>& pBuffer, DWORD nLength,
                                            DWORD &dwSourceTID, DWORD &dwDestinationTID, DWORD &nOutLength);

    static std::shared_ptr<BYTE> EncodeMsg(const std::shared_ptr<BYTE>& pBuffer, DWORD nLength,
                                           DWORD dwSourceTID, DWORD dwDestinationTID, DWORD &nOutLength);

    static std::shared_ptr<BYTE> DecodeMsg(const std::shared_ptr<BYTE>& pBuffer, DWORD nLength,
                                           DWORD &dwSourceTID, DWORD &dwDestinationTID, DWORD &nOutLength);

    static bool GetRouterInfo(const std::shared_ptr<BYTE>& pBuffer, DWORD nLength, DWORD &dwSourceTID, DWORD &dwDestinationTID);

};


#endif //NETCOMBTRANSFER_DATACODEC_H
