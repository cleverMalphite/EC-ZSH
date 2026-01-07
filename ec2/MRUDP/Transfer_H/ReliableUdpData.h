//
// Created by 王炳棋 on 2022/12/20.
//

#ifndef MRUDP_RELIABLEUDPDATA_H
#define MRUDP_RELIABLEUDPDATA_H

#include "SerializeUtil.h"
#include "../epollComm/CIPSOCKET.h"

namespace MRUDP {
    //RUDP数据包的有效数据部分长度
    const DWORD DATA_PACKET_LENGTH = 1300;

    /**
	 * 这个类属于实时UDP可靠传输的数据包实体
	 * 主要的结构如下：
	 * 1个字节的标志位 + 1字节的紧急传输标志 + 4个字节的包序号 + 4个字节的传输轮次 + 4个字节的时间戳 +
     * 1个字节的传输原因(1:初次发送；2：请求重传；3：超时传输) + 4个字节的数据部分长度 + 数据部分
	 * 需要注意的是，这个类不仅仅用于数据包的编解码，还用于构成循环队列文件的数组部分
	 */
    class ReliableUdpData {
    public:
        ReliableUdpData() {

        }

        ReliableUdpData(std::shared_ptr<BYTE> sData, DWORD dwDataLength) :
                m_uwDataLength(dwDataLength), m_udTimeStamp(0) {
            //深拷贝
            std::shared_ptr<BYTE> pTemp(MRUDP::ReadChar(sData.get(), dwDataLength), releaseArrays<BYTE>);
            m_transfer_reason = MRUDPTransferReason::TRANSFER_FOR_INIT;
            m_sData = pTemp;
        }

        virtual ~ReliableUdpData(void) {

        }

    protected:
        BYTE m_ucFlag = MRUDP_DATA_FIRST_BYTE_FLAG;
        DWORD m_dPacketIndex = 0;   //数据包在循环发送队列里的索引
        DWORD m_dCycleIndex = 0;    //传输轮次(第几次重复使用此包序号)
        DWORD m_udTimeStamp = 0;    //四个字节的生成时间戳
        DWORD m_uwDataLength = 0;   //四个字节的数据部分长度
        BYTE m_transfer_reason = 0; //为1表示初次发送，为2表示请求重传，为3表示超时重传
        std::shared_ptr<BYTE> m_sData = nullptr;
        BYTE m_bPacketUrgentFlag = 0;
    public:
        const static DWORD m_nPacketLengthExceptData = 19;
        const static DWORD m_nPacketLengthReason = 14;
        const static DWORD m_nPacketUrgentFlag = 1;
    public:
        std::shared_ptr<BYTE> Encode(DWORD &nLength) const;

        bool Decode(const std::shared_ptr<BYTE> &packetBytes, DWORD nLength);

        bool ReleaseData();

        bool AddToQueueSuccess(DWORD dPacketIndex, DWORD dCycleIndex);

        BYTE GetTransferReason() const { return m_transfer_reason; }

        BYTE GetPacketUrgentFlag() const { return m_bPacketUrgentFlag; }

        DWORD GetTimeStamp() const { return m_udTimeStamp; }

        DWORD GetDataLength() const { return m_uwDataLength; }

        DWORD GetPacketIndex() const { return m_dPacketIndex; }

        const DWORD GetCycleIndex() const { return m_dCycleIndex; }

        static DWORD GetPacketHeaderLength() { return m_nPacketLengthExceptData; }

        //MARK
        std::shared_ptr<BYTE> GetData() { return m_sData; }

        void SetDataNull() { m_sData = nullptr; }

        void SetPacketIndex(DWORD dwIndex) { m_dPacketIndex = dwIndex; }

        void SetCycleIndex(DWORD dwIndex) { m_dCycleIndex = dwIndex; }

    };


}

#endif //MRUDP_RELIABLEUDPDATA_H
