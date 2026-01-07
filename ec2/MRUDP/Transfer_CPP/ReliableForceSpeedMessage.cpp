//
// Created by 王炳棋 on 2022/12/28.
//
#include "Transfer_H/ReliableForceSpeedMessage.h"

namespace MRUDP {
    std::shared_ptr<BYTE> ReliableForceSpeedMessage::Encode(DWORD &nLength) const {
        if (MRUDP_FORCE_SPEED_MESSAGE == m_bFlag) {
            std::shared_ptr<BYTE> buffer_encode(new BYTE[m_nPacketLengthExceptData], releaseArrays<BYTE>);
            BYTE *temp_byte = buffer_encode.get();

            temp_byte[0] = m_bFlag;
            temp_byte += 1;

            WriteData(temp_byte, m_remote_tid);
            temp_byte += 4;

            WriteData(temp_byte, m_force_speed);

            nLength = m_nPacketLengthExceptData;
            return buffer_encode;
        }

        return nullptr;
    }

    bool ReliableForceSpeedMessage::Decode(const std::shared_ptr<BYTE>& packetByte, DWORD nLength) {
        BYTE *pTemp = packetByte.get();
        //读取数据标志
        if (packetByte && nLength >= m_nPacketLengthExceptData) {
            //检查命令与数据区别位
            if (pTemp[0] != MRUDP_FORCE_SPEED_MESSAGE) {
                return false;
            }
            m_bFlag = MRUDP_FORCE_SPEED_MESSAGE;
            pTemp += 1;

            m_remote_tid = ReadData(pTemp);
            pTemp += 4;

            m_force_speed = ReadData(pTemp);

            return true;
        }

        return false;
    }
}