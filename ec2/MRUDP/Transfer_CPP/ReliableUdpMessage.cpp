//
// Created by 王炳棋 on 2022/12/21.
//

#include "Transfer_H/ReliableUdpMessage.h"

namespace MRUDP {

    std::shared_ptr<unsigned char> Force_ACK_Message::Encode(DWORD &nLength) {

        if (MRUDP_MESSAGE_FIRST_BYTE_FLAG == m_bFlag) {
            //给返回的数据分配内存空间
            nLength = m_nPacketLengthExceptData + sizeof(DWORD) * m_sack_blocks.size();
            std::shared_ptr<BYTE> pData(new BYTE[nLength], releaseArrays<BYTE>);
            //开始顺序写入各个字段的值到上述字节序列中
            BYTE *pTemp = pData.get();
            //命令与数据区分开，表示这是数据
            pTemp[0] = m_bFlag;
            pTemp += 1;
            WriteData(pTemp, m_dTimeStamp);
            pTemp += 4;        //写入当前系统时间戳
            WriteData(pTemp, m_dTailPacketNumber);
            pTemp += 4;         //写入尾包序号
            WriteData(pTemp, m_dCycleIndex);
            pTemp += 4;          //写入尾包传输轮次
            WriteData(pTemp, m_dTailPacketTimeStamp);
            pTemp += 4;         //写入尾包时间戳
            WriteData(pTemp, m_dTailCumulativeAckTimeStamp);
            pTemp += 4;
            WriteData(pTemp, m_dTailFirstAckedTimeStamp);
            pTemp += 4;
            pTemp[0] = m_dTailReTransferForTimeout;
            pTemp += 1;
            pTemp[0] = m_dTailReTransferForRequest;
            pTemp += 1;
            WriteData(pTemp, m_dSackBlockNumber);
            pTemp += 4;
            for (auto sacks: m_sack_blocks) {
                WriteData(pTemp, sacks);
                pTemp += 4;
            }
            return pData;
        }

        return nullptr;
    }

    bool Force_ACK_Message::Decode(const std::shared_ptr<BYTE>& packet_bytes, DWORD nLength) {
        BYTE *pTemp = packet_bytes.get();
        //读取数据标志
        if (packet_bytes && nLength >= m_nPacketLengthExceptData) {
            //检查命令与数据区别位
            if (pTemp[0] != MRUDP_MESSAGE_FIRST_BYTE_FLAG) {
                return false;
            }
            m_bFlag = MRUDP_MESSAGE_FIRST_BYTE_FLAG;
            pTemp += 1;

            m_dTimeStamp = ReadData(pTemp);
            pTemp += 4;        //写入当前系统时间戳
            m_dTailPacketNumber = ReadData(pTemp);
            pTemp += 4;//写入尾包序号
            m_dCycleIndex = ReadData(pTemp);
            pTemp += 4;    //写入尾包轮次

            /*if (m_dCycleIndex != 0) {*/
            m_dTailPacketTimeStamp = ReadData(pTemp);
            pTemp += 4;    //写入尾包时间戳
            m_dTailCumulativeAckTimeStamp = ReadData(pTemp);
            pTemp += 4;
            m_dTailFirstAckedTimeStamp = ReadData(pTemp);
            pTemp += 4;
            m_dTailReTransferForTimeout = pTemp[0];
            pTemp += 1;
            m_dTailReTransferForRequest = pTemp[0];
            pTemp += 1;
            m_dSackBlockNumber = ReadData(pTemp);
            pTemp += 4;
            DWORD sack_tmp = 0;
            for (DWORD i = 0; i < m_dSackBlockNumber /*&& pTemp*/; i++) {
                sack_tmp = ReadData(pTemp);
                pTemp += 4;
                m_sack_blocks.push_back(sack_tmp);
            }
            /*}*/
            return true;
        }

        return false;
    }
}
