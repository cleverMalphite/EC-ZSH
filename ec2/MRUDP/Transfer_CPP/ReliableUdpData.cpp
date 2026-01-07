//
// Created by 王炳棋 on 2022/12/21.
//

#include "Transfer_H/ReliableUdpData.h"

namespace MRUDP {

    std::shared_ptr<BYTE> ReliableUdpData::Encode(DWORD &nLength) const {

        //printf("New Encode\n");
        nLength = m_nPacketLengthExceptData + m_uwDataLength;
        std::shared_ptr<BYTE> pData(new BYTE[nLength], releaseArrays<BYTE>);

        BYTE *pTemp = pData.get();
        pTemp[0] = MRUDP_DATA_FIRST_BYTE_FLAG;
        pTemp += 1;
        pTemp[0] = 0;
        pTemp += 1;
        WriteData(pTemp, m_dPacketIndex);
        pTemp += 4;
        WriteData(pTemp, m_dCycleIndex);
        pTemp += 4;
        WriteData(pTemp, m_udTimeStamp);
        pTemp += 4;
        pTemp[0] = m_transfer_reason;
        pTemp += 1;
        WriteData(pTemp, m_uwDataLength);
        pTemp += 4;
        WriteChar(pTemp, m_sData.get(), m_uwDataLength);
        //pTemp += m_uwDataLength;
        return pData;
    }

    bool ReliableUdpData::Decode(const std::shared_ptr<BYTE>& packetBytes, DWORD nLength) {
        BYTE *pTemp = packetBytes.get();
        //读取数据标志
        if (packetBytes && nLength >= m_nPacketLengthExceptData) {
            //检查命令与数据区别位
            if (pTemp[0] != MRUDP_DATA_FIRST_BYTE_FLAG) {
                return false;
            }
            pTemp += 1;
            m_bPacketUrgentFlag = pTemp[0];
            pTemp += 1;
            m_dPacketIndex = ReadData(pTemp);
            pTemp += 4;    //读取包序号
            m_dCycleIndex = ReadData(pTemp);
            pTemp += 4;    //写入包传输轮次
            m_udTimeStamp = ReadData(pTemp);
            pTemp += 4;    //读取时间戳
            m_transfer_reason = pTemp[0];
            pTemp += 1;    //读取传输原因
            m_uwDataLength = ReadData(pTemp);
            pTemp += 4;    //读取有效数据部分的长度
            if (m_uwDataLength != (nLength - m_nPacketLengthExceptData)) {
                printf("[MRUDPDecode]::packet data length wrong! ExpectDataLength is %d\n",m_uwDataLength);
                return false;
            }
            std::shared_ptr<BYTE> pData(ReadChar(pTemp, m_uwDataLength), releaseArrays<BYTE>);
            pTemp += (nLength - m_nPacketLengthExceptData);    //读取有效数据部分
            m_sData = pData;
        }
        return true;
    }

    bool ReliableUdpData::ReleaseData() { return true; }

    bool ReliableUdpData::AddToQueueSuccess(DWORD dPacketIndex, DWORD dCycleIndex) {
        m_udTimeStamp = GetTickCount();
        m_dPacketIndex = dPacketIndex;
        m_dCycleIndex = dCycleIndex;
        return true;
    }
}
