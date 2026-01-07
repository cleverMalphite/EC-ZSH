//
// Created by 王炳棋 on 2022/12/7.
//

#include "DataWithPacketInfo.h"

//获取时间戳，单位为ms

namespace SpeedControl {
    typedef struct UtcTimeStampInSpeedControl {
        DWORD dwHighDateTime = 0;
        DWORD dwLowDateTime = 0;
    } UtcTimeStampInSpeedControl;

    //获取64位UTC时间(毫秒)
    UtcTimeStampInSpeedControl su_get_sys_time_ms(int64_t &timeStamp_get) {
        timeval timeNow;
        gettimeofday(&timeNow, nullptr);
        timeStamp_get = (timeNow.tv_sec * 1000000 + timeNow.tv_usec) / 1000;
        struct UtcTimeStampInSpeedControl timeStamp;
        int64_t int_temp_first = timeStamp_get;
        int64_t int_temp_second = timeStamp_get;
        timeStamp.dwLowDateTime = int_temp_first & 0x00000000ffffffff;
        timeStamp.dwHighDateTime = (int_temp_second >> 32) & 0x00000000ffffffff;
        return timeStamp;
    }

    DataWithPacketInfo::DataWithPacketInfo(DWORD dest_tid, const SelfToRemoteQosInfo &self_to_remote,
                                           std::shared_ptr<BYTE> data, const DWORD data_length, bool isRBUDP)
            : m_dest_tid(
            dest_tid) {
        m_data_packet_info_self_to_remote->m_d_remote_tid = dest_tid;
        m_data_packet_info_self_to_remote->m_d_index = self_to_remote.m_d_index;
        //m_data_packet_info_self_to_remote->m_d_timestamp = su_get_sys_time_ms();
        m_data_packet_info_self_to_remote->m_d_data_length = data_length;
        m_data = data;
        m_data_length = data_length;
        m_isRBUDP = isRBUDP;
    }

    bool DataWithPacketInfo::IsFeedBackInstanceOf(shared_ptr<BYTE> pBuffer, DWORD dLength) {
        if (nullptr != pBuffer && dLength > 1) {
            BYTE *pTemp = pBuffer.get();
            if (m_d_feedback_comman_flag == pTemp[0]) {
                return true;
            }
        }
        return false;
    }

    bool DataWithPacketInfo::Decode(shared_ptr<BYTE> s_data, DWORD dLength) {
        if (nullptr == s_data || dLength < MIN_PACKET_DATA_LENGTH) {
            return false;
        }
        BYTE *pTemp = s_data.get();
        if (NULL == pTemp) {
            return false;
        }
        UtcTimeStampInSpeedControl utc_stamp_temp;
        m_data_packet_info_remote_to_self->m_d_remote_tid = m_dest_tid;
        m_data_packet_info_remote_to_self->m_d_index = CIMsg::ReadData(pTemp);
        pTemp += 4;
        utc_stamp_temp.dwHighDateTime = CIMsg::ReadData(pTemp);
        pTemp += 4;
        utc_stamp_temp.dwLowDateTime = CIMsg::ReadData(pTemp);
        pTemp += 4;
        m_data_packet_info_remote_to_self->m_d_timestamp =
                ((((int64_t) utc_stamp_temp.dwHighDateTime)) << 32) | utc_stamp_temp.dwLowDateTime;
        m_data_length = CIMsg::ReadData(pTemp);
        pTemp += 4;
        m_isRBUDP = CIMsg::ReadData(pTemp);
        pTemp += 4;
        m_data_packet_info_remote_to_self->m_d_datalength = m_data_length;
        shared_ptr<BYTE> p_temp_data(new BYTE[m_data_length], releaseArrays<BYTE>);
        memcpy(p_temp_data.get(), pTemp, m_data_length);
        m_data = p_temp_data;

        return true;
    }

    std::shared_ptr<BYTE> DataWithPacketInfo::Encode(DWORD &dLength) {
        dLength = 0;
        shared_ptr<BYTE> p_data(new BYTE[m_data_length + MIN_PACKET_DATA_LENGTH], releaseArrays<BYTE>);
        BYTE *pTemp = p_data.get();
        if (nullptr != pTemp) {
            //还是要更新一下时间戳！！！
            UtcTimeStampInSpeedControl utc_stamp_temp = su_get_sys_time_ms(
                    m_data_packet_info_self_to_remote->m_d_timestamp);
            dLength = m_data_length + MIN_PACKET_DATA_LENGTH;
            CIMsg::WriteData(pTemp, m_data_packet_info_self_to_remote->m_d_index);
            pTemp += 4;
            CIMsg::WriteData(pTemp, utc_stamp_temp.dwHighDateTime);
            pTemp += 4;   //时间戳高四位
            CIMsg::WriteData(pTemp, utc_stamp_temp.dwLowDateTime);
            pTemp += 4;    //时间戳低四位
            CIMsg::WriteData(pTemp, m_data_length);
            pTemp += 4;
            CIMsg::WriteData(pTemp, m_isRBUDP);
            pTemp += 4;
            memcpy(pTemp, m_data.get(), m_data_length);
            return p_data;
        } else {
            return nullptr;
        }
    }
}
