//
// Created by Kong on 2023/6/9.
//
#include "Transfer_H/RBUDPdata.h"

namespace RBUDP
{

    std::shared_ptr<unsigned char> RBUDPdata::Encode(DWORD& nLength) const {
        nLength = m_nPacketLengthExceptData + m_uwDataLength;
        std::shared_ptr<BYTE> encodeChar(new BYTE[nLength], releaseArrays<BYTE>);

        //将成员对象的数据写入刚刚开辟的空间中
        BYTE* pTemp = encodeChar.get();

        pTemp[0] = MRUDP_DATA_FIRST_BYTE_FLAG;	pTemp += 1;
        pTemp[0] = 0;				pTemp += 1;
        WriteData(pTemp, m_dLength_all);		pTemp += 4;
        WriteData(pTemp, m_dPacketIndex);		pTemp += 4;			//写入包序号
        WriteData(pTemp, m_udTimeStamp);		pTemp += 4;
        //pTemp[0] = m_transfer_reason;			pTemp += 1;
        WriteData(pTemp, m_uwDataLength);		pTemp += 4;
        WriteChar(pTemp, m_sData.get(), m_uwDataLength);	pTemp += m_uwDataLength;
        return encodeChar;
    }

    bool RBUDPdata::Decode(std::shared_ptr<BYTE> packet_bytes, DWORD nLength) {
        BYTE* pTemp = packet_bytes.get();
        //读取数据标志
        if (packet_bytes && nLength >= m_nPacketLengthExceptData)
        {
            //检查命令与数据区别位
            if (pTemp[0] != MRUDP_DATA_FIRST_BYTE_FLAG)
            {
                return false;
            }
            pTemp += 1;
            m_bPacketUrgentFlag = pTemp[0];		pTemp++;
            m_dLength_all = ReadData(pTemp);	pTemp += 4;
            m_dPacketIndex = ReadData(pTemp);	pTemp += 4;	//读取包序号
            m_udTimeStamp = ReadData(pTemp);	pTemp += 4;	//读取时间戳
            //m_transfer_reason = *pTemp;			pTemp += 1;	//读取传输原因
            m_uwDataLength = ReadData(pTemp);	pTemp += 4;	//读取有效数据部分的长度
            if (m_uwDataLength != (nLength - m_nPacketLengthExceptData))
            {
                //SPDLOG_LOGGER_WARN(mrudp_logger, "packet data length wrong! PacketIndex, {}; CycleIndex, {}; uwDataLength, {}; realLength, {}!",
                //m_dPacketIndex, m_dCycleIndex, m_uwDataLength, (nLength - m_nPacketLengthExceptData));
                return false;
            }
            std::shared_ptr<BYTE> pTempSharedPtr(ReadChar(pTemp, m_uwDataLength), releaseArrays<BYTE>);  pTemp += (nLength - m_nPacketLengthExceptData);	//读取有效数据部分
            m_sData = pTempSharedPtr;
        }
        return true;
    }
    bool RBUDPdata::ReleaseData()
    {
        return true;
    }
    bool RBUDPdata::AddToQueueSuccess(DWORD dPacketIndex, DWORD dLengthall)
    {
        m_dLength_all = dLengthall;
        m_udTimeStamp = GetTickCount();
        m_dPacketIndex = dPacketIndex;
        return true;
    }

}
