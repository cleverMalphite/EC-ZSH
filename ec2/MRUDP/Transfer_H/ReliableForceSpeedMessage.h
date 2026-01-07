//
// Created by 王炳棋 on 2022/12/28.
//
#include "SerializeUtil.h"
#include "../epollComm/CIPSOCKET.h"

#ifndef NETCOMBTRANSFER_RELIABLEFORCESPEEDMESSAGE_H
#define NETCOMBTRANSFER_RELIABLEFORCESPEEDMESSAGE_H

namespace MRUDP {
/**
* 接收端调整发送端到此终端的发送速率
* 1字节标志+4字节对端终端号+4字节要求调整到的速率
*/
    class ReliableForceSpeedMessage {
    public:
        const static DWORD m_nPacketLengthExceptData = 9;

        ReliableForceSpeedMessage(DWORD remote_tid, DWORD force_speed) :
                m_remote_tid(remote_tid), m_force_speed(force_speed),
                m_bFlag(MRUDP_FORCE_SPEED_MESSAGE) {

        }

        ReliableForceSpeedMessage() : m_bFlag(MRUDP_FORCE_SPEED_MESSAGE) {

        }

    public:

        std::shared_ptr<BYTE> Encode(DWORD &nLength) const;    //将数据包序列化，并将字节序列化后字节序列的有效长度通过参数传递给调用方

        bool Decode(const std::shared_ptr<BYTE>& packetBytes, DWORD nLength);    //将字节序列反序列化

        DWORD GetRemoteTid() const {
            return m_remote_tid;
        }

        DWORD GetForceSpeed() const {
            return m_force_speed;
        }

    private:
        BYTE m_bFlag;    //标志位
        DWORD m_remote_tid;    //要求调整m_remote_tid的发送速率
        DWORD m_force_speed;    //要求调整到的速率（bps）
    };
}

#endif //NETCOMBTRANSFER_RELIABLEFORCESPEEDMESSAGE_H
