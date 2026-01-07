//
// Created by 王炳棋 on 2022/12/28.
//
#include "SerializeUtil.h"
#include "../epollComm/CIPSOCKET.h"

#ifndef NETCOMBTRANSFER_RELIABLEFORCEADJUSTBANDWIDTHMESSAGE_H
#define NETCOMBTRANSFER_RELIABLEFORCEADJUSTBANDWIDTHMESSAGE_H
namespace MRUDP {
/**
* 接收端要求发送端进行大规模紧急重传
* 1字节标志+4字节对端终端号
*/
    class ReliableForceAdjustBandWidthMessage {
    public:
        const static DWORD m_nPacketLengthExceptData = 5;

        ReliableForceAdjustBandWidthMessage(DWORD remote_tid)
                : m_remote_tid(remote_tid), m_bFlag(MRUDP_FORCE_ADJUSTBANDWIDTH_MESSAGE) {}

        ReliableForceAdjustBandWidthMessage() : m_bFlag(MRUDP_FORCE_ADJUSTBANDWIDTH_MESSAGE) {}

    public:

        std::shared_ptr<BYTE> Encode(DWORD &nLength) const;    //将数据包序列化，并将字节序列化后字节序列的有效长度通过参数传递给调用方

        bool Decode(const std::shared_ptr<BYTE>& packetBytes, DWORD nLength);    //将字节序列反序列化

        DWORD GetRemoteTid() const {
            return m_remote_tid;
        }

    private:
        BYTE m_bFlag;    //标志位
        DWORD m_remote_tid;    //要求调整m_remote_tid的发送速率
    };
}
#endif //NETCOMBTRANSFER_RELIABLEFORCEADJUSTBANDWIDTHMESSAGE_H
