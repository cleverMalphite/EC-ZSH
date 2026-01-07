//
// Created by Kong on 2023/6/9.
//

#include "Transfer_H/NAK_Message.h"
#include "../epollComm/CIPSOCKET.h"

namespace RBUDP
{
    std::shared_ptr<unsigned char> NAK_Message::Encode(DWORD& nlength)
    {
        if (MRUDP_MESSAGE_FIRST_BYTE_FLAG == m_bFlag)
        {
            //给返回的数据分配内存空间
            nlength = m_nPacketLengthExceptData + sizeof(DWORD)*m_sack_blocks.size();
            std::shared_ptr<BYTE> encodeChar(new BYTE[nlength], releaseArrays<BYTE>);
            //开始顺序写入各个字段的值到上述字节序列中
            BYTE* pTemp = encodeChar.get();
            //命令与数据区分开，表示这是数据
            pTemp[0] = m_bFlag;	pTemp += 1;

            pTemp[0] = m_fFlag;			pTemp += 1;
            WriteData(pTemp, m_doffset);			pTemp += 4;		//写入当前系统时间戳
            WriteData(pTemp, m_dSackBlockNumber);	pTemp += 4;
            if (m_dSackBlockNumber == 0) {
                //SPDLOG_LOGGER_WARN(mrudp_logger, "fack encode wrong: m_dTailPacketNumber, {}; m_dCycleIndex, {}; m_dSackBlockNumber, {}",
                //m_dTailPacketNumber, m_dCycleIndex, m_dSackBlockNumber);
            }
            int i = 0;
            for (auto iter = m_sack_blocks.begin(); iter != m_sack_blocks.end(); iter++)
            {
                WriteData(pTemp, *iter); pTemp += 4;
                //SPDLOG_LOGGER_WARN(rbudp_logger, "{}:{}", i++, *iter);
            }
            return encodeChar;
        }

        return nullptr;
    }

    bool NAK_Message::Decode(const std::shared_ptr<BYTE> packet_bytes, DWORD nLength)
    {
        BYTE*  pTemp = packet_bytes.get();
        //读取数据标志
        if (packet_bytes && nLength >= m_nPacketLengthExceptData)
        {
            //检查命令与数据区别位
            if (pTemp[0] != MRUDP_MESSAGE_FIRST_BYTE_FLAG)
            {
                return false;
            }
            m_bFlag = MRUDP_MESSAGE_FIRST_BYTE_FLAG;
            pTemp += 1;

            m_fFlag = pTemp[0];				pTemp += 1;
            m_doffset = ReadData(pTemp);			pTemp += 4;		//写入偏移量
            //m_dTailPacketNumber = ReadData(pTemp);			pTemp += 4;//写入尾包序号
            //m_dCycleIndex = ReadData(pTemp);			pTemp += 4;	//写入尾包重传轮次
            m_dSackBlockNumber = ReadData(pTemp);	pTemp += 4;
            DWORD sack_tmp;
            if (m_dSackBlockNumber == 0)
            {
                //SPDLOG_LOGGER_WARN(rbudp_rm_logger, "nak decode wrong: m_FFlag, {}; m_doffset, {}; m_dSackBlockNumber, {}",
                //m_fFlag, m_doffset, m_dSackBlockNumber);
                return false;
            }
            for (DWORD i = 0; i < m_dSackBlockNumber&&pTemp; i++)
            {
                sack_tmp = ReadData(pTemp); pTemp += 4;
                m_sack_blocks.push_back(sack_tmp);
                //SPDLOG_LOGGER_WARN(rbudp_rs_logger, "{}:{}", i, sack_tmp);
            }
            return true;
        }

        return false;
    }
}
