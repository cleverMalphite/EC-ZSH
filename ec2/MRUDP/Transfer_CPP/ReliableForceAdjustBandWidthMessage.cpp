//
// Created by 王炳棋 on 2022/12/28.
//
#include "Transfer_H/ReliableForceAdjustBandWidthMessage.h"

namespace MRUDP {
    std::shared_ptr<BYTE> ReliableForceAdjustBandWidthMessage::Encode(DWORD &nLength) const {
        if (MRUDP_FORCE_ADJUSTBANDWIDTH_MESSAGE == m_bFlag) {
            std::shared_ptr<BYTE> buffer_encode(new BYTE[m_nPacketLengthExceptData], releaseArrays<BYTE>);
            BYTE *temp_byte = buffer_encode.get();
            temp_byte[0] = m_bFlag;
            temp_byte += 1;

            WriteData(temp_byte, m_remote_tid);

            nLength = m_nPacketLengthExceptData;
            return buffer_encode;
        }

        return nullptr;
    }

    bool ReliableForceAdjustBandWidthMessage::Decode(const std::shared_ptr<BYTE>& packetBytes, DWORD nLength) {
        BYTE *pTemp = packetBytes.get();
        //读取数据标志
        if (packetBytes && nLength >= m_nPacketLengthExceptData) {
            //检查命令与数据区别位
            if (pTemp[0] != MRUDP_FORCE_ADJUSTBANDWIDTH_MESSAGE) {
                return false;
            }

            m_bFlag = MRUDP_FORCE_ADJUSTBANDWIDTH_MESSAGE;
            pTemp += 1;

            m_remote_tid = ReadData(pTemp);
            
            return true;
        }

        return false;
    }

}