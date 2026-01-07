//
// Created by 王炳棋 on 2023/1/11.
//
#include "DataAndMessage_H/FileRecvSuccess.h"

namespace BigDataTransfer {
    std::shared_ptr<BYTE> FileRecvSuccess::Encode(DWORD &dDataLength) {
        dDataLength = m_packet_length;
        //分配内存空间
        std::shared_ptr<BYTE> pBuffer(new BYTE[m_packet_length], releaseArrays<BYTE>);
        //开始序列化数据
        BYTE *pTemp = pBuffer.get();
        pTemp[0] = m_flag;
        pTemp++;
        CIMsg::WriteData(pTemp, m_file_transfer_task_id);
        return pBuffer;
    }

    bool FileRecvSuccess::Decode(std::shared_ptr<BYTE> pBuffer, DWORD dLength) {
        BYTE *pTemp = pBuffer.get();
        if (m_flag != pTemp[0] || m_packet_length != dLength) {
            return false;
        }

        pTemp++;
        m_file_transfer_task_id = CIMsg::ReadData(pTemp);
        return true;
    }

    bool FileRecvSuccess::IsEncodeInstanceOf(std::shared_ptr<BYTE> pData, DWORD dDataLength) {
        BYTE *pTemp = pData.get();
        if (m_flag != pTemp[0] || m_packet_length != dDataLength) {
            return false;
        } else {
            return true;
        }
    }
}