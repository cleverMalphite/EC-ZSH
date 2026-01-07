//
// Created by 王炳棋 on 2023/1/11.
//

#include "DataAndMessage_H/FileTransferData.h"

namespace BigDataTransfer {
    std::shared_ptr<BYTE> FileTransferData::Encode(DWORD &dDataLength) {
        //1. 先分配字节空间
        dDataLength = m_packet_header_length + m_file_name_length + m_data_length;
        std::shared_ptr<BYTE> pBuffer(new BYTE[dDataLength], releaseArrays<BYTE>);
        BYTE *pTemp = pBuffer.get();
        //2. 开始序列化包头
        pTemp[0] = m_dFlag;
        pTemp++;
        CIMsg::WriteData(pTemp, m_file_transfer_task_id);
        pTemp += 4;
        CIMsg::WriteData64(pTemp, m_file_length_all);
        pTemp += 8;
        CIMsg::WriteData(pTemp, m_file_packet_number);
        pTemp += 4;
        CIMsg::WriteData64(pTemp, m_file_already_send_length);
        pTemp += 8;
        CIMsg::WriteData(pTemp, m_file_name_length);
        pTemp += 4;
        CIMsg::WriteData(pTemp, m_data_length);
        pTemp += 4;
        CIMsg::WriteData(pTemp, m_packet_has_transfered_number);
        pTemp += 4;
        //3. 开始序列化包尾
        CIMsg::WriteChar(pTemp, (BYTE *) (m_file_name.c_str()), m_file_name_length);
        pTemp += m_file_name_length;
        //printf("%d\n",m_file_name_length);
        CIMsg::WriteChar(pTemp, m_data.get(), m_data_length);
        pTemp += m_data_length;

        return pBuffer;
    }

    bool FileTransferData::Decode(std::shared_ptr<BYTE> pBuffer, DWORD dLength) {
        BYTE *pTemp = pBuffer.get();

        //先反序列化包头的数据
        if (m_dFlag != pTemp[0]) {
            return false;
        }
        pTemp++;
        m_file_transfer_task_id = CIMsg::ReadData(pTemp);
        pTemp += 4;
        m_file_length_all = CIMsg::ReadData64(pTemp);
        pTemp += 8;
        m_file_packet_number = CIMsg::ReadData(pTemp);
        pTemp += 4;
        m_file_already_send_length = CIMsg::ReadData64(pTemp);
        pTemp += 8;
        m_file_name_length = CIMsg::ReadData(pTemp);
        pTemp += 4;
        m_data_length = CIMsg::ReadData(pTemp);
        pTemp += 4;
        m_packet_has_transfered_number = CIMsg::ReadData(pTemp);
        pTemp += 4;
        //开始反序列化包尾
        std::shared_ptr<BYTE> p_file_name(new BYTE[m_file_name_length], releaseArrays<BYTE>);
        memcpy(p_file_name.get(), pTemp, m_file_name_length);
        pTemp += m_file_name_length;
        m_file_name = (char *) p_file_name.get();
        std::shared_ptr<BYTE> p_temp_data(new BYTE[m_data_length], releaseArrays<BYTE>);
        m_data = p_temp_data;
        memcpy(m_data.get(), pTemp, m_data_length);
        return true;
    }

    bool FileTransferData::IsEncodeInstanceOf(std::shared_ptr<BYTE> pData, DWORD dDataLength) {
        if (dDataLength <= 0) {
            return false;
        }
        BYTE bFlag = pData.get()[0];
        if (m_dFlag == bFlag) {
            return true;
        } else {
            return false;
        }
    }
}