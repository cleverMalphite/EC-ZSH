//
// Created by 王炳棋 on 2023/1/11.
//
#include "DataAndMessage_H/FileRecvFailed.h"

namespace BigDataTransfer {

    FileRecvFailed::FileRecvFailed(DWORD d_task_id) : m_file_transfer_task_id(d_task_id) {

    }

    FileRecvFailed::FileRecvFailed() : m_file_transfer_task_id(0) {

    }

    std::shared_ptr<BYTE> FileRecvFailed::Encode(DWORD &dDataLength) {
        dDataLength = m_packet_length;
        //分配内存空间
        std::shared_ptr<BYTE> pBuffer(new BYTE[m_packet_length], releaseArrays<BYTE>);
        //开始序列化数据
        BYTE *pTemp = pBuffer.get();
        pTemp[0] = m_flag;
        pTemp++;
        CIMsg::WriteData(pTemp, m_file_transfer_task_id);
        pTemp += 4;
        return pBuffer;
    }

    bool FileRecvFailed::Decode(std::shared_ptr<BYTE> pBuffer, DWORD dLength) {
        BYTE *pTemp = pBuffer.get();
        if (m_flag != pTemp[0] || m_packet_length != dLength) {
            return false;
        }

        pTemp++;
        m_file_transfer_task_id = CIMsg::ReadData(pTemp);
        pTemp += 4;
        return true;
    }

    bool FileRecvFailed::IsEncodeInstanceOf(std::shared_ptr<BYTE> pData, DWORD dDataLength) {
        BYTE *pTemp = pData.get();
        if (m_flag != pTemp[0] || m_packet_length != dDataLength) {
            return false;
        } else {
            return true;
        }
    }
}