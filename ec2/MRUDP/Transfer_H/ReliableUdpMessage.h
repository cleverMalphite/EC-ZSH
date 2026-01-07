//
// Created by 王炳棋 on 2022/12/21.
//

#include <queue>
#include "SerializeUtil.h"
#include "Store_H/ReliableUdpDataReceiverStore.h"

#ifndef NETCOMBTRANSFER_RELIABLEUDPMESSAGE_H
#define NETCOMBTRANSFER_RELIABLEUDPMESSAGE_H

namespace MRUDP {
    /**
	 * 这个类是RUDP信令的抽象，简单地说就是FACK和TACK报文的抽象（详见设计文档V2.1）
	 * 结构：数据命令区别位(1B) + 时间戳(4B) + 尾包序号(4B) +
     * 尾包时间戳(4B) +  尾包累积确认时间(4B) + 尾包首次确认时间(4B) +
     * 尾包超时重传次数(1B) + 尾包请求重传次数(1B) + 丢包区间个数(4B) + 丢包区间集合[n*8B]
	 * 时间戳指的是所传输的尾包序号对应的尾包的ACK。
	 */
    class Force_ACK_Message {
    public:
        Force_ACK_Message(const DWORD dTimeStamp, const std::shared_ptr<ReliableUdpDataReceiverStore> &receiveStore,
                          const std::vector<DWORD> &sack_blocks) :
                m_bFlag(MRUDP_MESSAGE_FIRST_BYTE_FLAG), m_dTimeStamp(dTimeStamp),
                m_dSackBlockNumber(sack_blocks.size()), m_sack_blocks(sack_blocks) {

            m_dTailPacketNumber = receiveStore->GetPacketIndex();
            m_dTailPacketTimeStamp = receiveStore->GetTimeStamp();
            m_dTailCumulativeAckTimeStamp = receiveStore->GetCumulativeTimeStamp();
            m_dTailFirstAckedTimeStamp = receiveStore->GetSackedTimeStamp();
            m_dTailReTransferForTimeout = receiveStore->GetTimeOutReTransferTime();
            m_dTailReTransferForRequest = receiveStore->GetReuqestReTransferTime();
            m_dCycleIndex = receiveStore->GetCycleIndex();
        }

        Force_ACK_Message(const DWORD dTimeStamp, ReliableUdpDataReceiverStore &receiveStore,
                          const std::vector<DWORD> &sack_blocks) :
                m_bFlag(MRUDP_MESSAGE_FIRST_BYTE_FLAG), m_dTimeStamp(dTimeStamp),
                m_dSackBlockNumber(sack_blocks.size()), m_sack_blocks(sack_blocks) {

            m_dTailPacketNumber = receiveStore.GetPacketIndex();
            m_dTailPacketTimeStamp = receiveStore.GetTimeStamp();
            m_dTailCumulativeAckTimeStamp = receiveStore.GetCumulativeTimeStamp();
            m_dTailFirstAckedTimeStamp = receiveStore.GetSackedTimeStamp();
            m_dTailReTransferForTimeout = receiveStore.GetTimeOutReTransferTime();
            m_dTailReTransferForRequest = receiveStore.GetReuqestReTransferTime();
            m_dCycleIndex = receiveStore.GetCycleIndex();
        }

        Force_ACK_Message(const DWORD dTimeStamp, DWORD dPacketIndex, DWORD dCycleIndex,
                          const std::vector<DWORD> &sack_blocks) :
                m_bFlag(MRUDP_MESSAGE_FIRST_BYTE_FLAG), m_dTimeStamp(dTimeStamp),
                m_dSackBlockNumber(sack_blocks.size()), m_sack_blocks(sack_blocks) {

            m_dTailPacketNumber = dPacketIndex;
            m_dCycleIndex = dCycleIndex;
            m_dTailPacketTimeStamp = 0;
            m_dTailCumulativeAckTimeStamp = 0;
            m_dTailFirstAckedTimeStamp = 0;
            m_dTailReTransferForTimeout = 0;
            m_dTailReTransferForRequest = 0;
        }

        Force_ACK_Message() :
                m_bFlag(MRUDP_MESSAGE_FIRST_BYTE_FLAG),
                m_dTimeStamp(GetTickCount()), m_dTailPacketNumber(0), m_dCycleIndex(0),
                m_dTailPacketTimeStamp(0),
                m_dTailCumulativeAckTimeStamp(0), m_dTailFirstAckedTimeStamp(0),
                m_dTailReTransferForTimeout(0), m_dTailReTransferForRequest(0),
                m_dSackBlockNumber(0) {

        }

        ~Force_ACK_Message() = default;

    private:

        BYTE m_bFlag;
        DWORD m_dTimeStamp = 0;
        DWORD m_dTailPacketNumber = 0;
        DWORD m_dCycleIndex = 0;
        DWORD m_dTailPacketTimeStamp = 0;
        DWORD m_dTailCumulativeAckTimeStamp = 0;
        DWORD m_dTailFirstAckedTimeStamp = 0;
        BYTE m_dTailReTransferForTimeout = 0;
        BYTE m_dTailReTransferForRequest = 0;
        DWORD m_dSackBlockNumber = 0;
        std::vector<DWORD> m_sack_blocks;
    public:

        const static DWORD m_nPacketLengthExceptData = 31;
    public:

        std::shared_ptr<BYTE> Encode(DWORD &nLength);    //将数据包序列化，并将字节序列化后字节序列的有效长度通过参数传递给调用方

        bool Decode(const std::shared_ptr<BYTE> &packet_bytes, DWORD nLength);    //将字节序列反序列化

        DWORD GetTimeStamp() const { return m_dTimeStamp; }

        DWORD GetTailIndexNumber() const { return m_dTailPacketNumber; }

        void SetTimeStamp(DWORD dTimeStamp) { m_dTimeStamp = dTimeStamp; }

        DWORD GetTailPacketTimeStamp() const { return m_dTailPacketTimeStamp; }

        std::vector<DWORD> GetSackBlocks() { return m_sack_blocks; }

        DWORD GetReceiveTimes() const {
            DWORD retransfer_time = m_dTailReTransferForRequest + m_dTailReTransferForTimeout;
            if (0 == retransfer_time) {
                retransfer_time = 1;
            }

            return retransfer_time;
        }

        DWORD GetTailCycleIndex() const { return m_dCycleIndex; }


    };
}

#endif //NETCOMBTRANSFER_RELIABLEUDPMESSAGE_H
