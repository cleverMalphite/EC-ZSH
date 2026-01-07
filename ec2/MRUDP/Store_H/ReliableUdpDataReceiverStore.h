//
// Created by 王炳棋 on 2022/12/21.
//
#include "Transfer_H/ReliableUdpData.h"

#ifndef NETCOMBTRANSFER_RELIABLEUDPDATARECEIVERSTORE_H
#define NETCOMBTRANSFER_RELIABLEUDPDATARECEIVERSTORE_H

namespace MRUDP {
    class ReliableUdpDataReceiverStore : public ReliableUdpData {
    private:
        BYTE m_timeout_retransfer_time = 0; //因为超时重传规程而导致接收端接收到此数据包的次数
        BYTE m_request_retransfer_time = 0; //因为请求重传规程而导致接收端接收到此数据包的次数
        BYTE m_priority = 0;    //包传输优先级
        BYTE m_ack_kind = MRUDPAckKind::UNSACKED;   //此包的确认情况
        DWORD m_cumulative_timestamp = 0;   //此包得到累积确认的时间戳
        DWORD m_acked_timestamp = 0;    //此数据包第一次被接收时的时间戳
    public:
        BYTE GetTimeOutReTransferTime() const {
            return m_timeout_retransfer_time;
        }

        BYTE GetReuqestReTransferTime() const {
            return m_request_retransfer_time;
        }


        bool AddCycleIndex() {
            m_dCycleIndex++;
            return true;
        }

        void AddTimeOutReTransferTime() {
            m_timeout_retransfer_time++;
        }

        void AddRequestRetransferTime() {
            m_request_retransfer_time++;
        }

        void SetSacked() {
            if (0 == m_acked_timestamp) {
                m_acked_timestamp = GetTickCount();
            }
            m_ack_kind = MRUDPAckKind::SACKED;
        }

        void SetCumulativeAcked() {
            if (0 == m_cumulative_timestamp) {
                m_cumulative_timestamp = GetTickCount();
            }
            m_ack_kind = MRUDPAckKind::CUMULATIVE_ACKED;
        }

        void SetUnAcked() {
            m_ack_kind = MRUDPAckKind::UNSACKED;
        }

        BYTE GetPriority() const {
            return m_priority;
        }

        DWORD GetCumulativeTimeStamp() const {
            return m_cumulative_timestamp;
        }

        DWORD GetSackedTimeStamp() const {
            return m_acked_timestamp;
        }

        bool IsSacked() const {
            if (m_ack_kind == MRUDPAckKind::SACKED) {
                return true;
            } else {
                return false;
            }
        }

        bool IsCumulative_Acked() const {
            if (m_ack_kind == MRUDPAckKind::CUMULATIVE_ACKED) {
                return true;
            } else {
                return false;
            }
        }

        bool GetTransferPriority() const {
            return m_priority;
        }

        bool IsAcked() const {
            return IsSacked() || IsCumulative_Acked();
        }

    };
}

#endif //NETCOMBTRANSFER_RELIABLEUDPDATARECEIVERSTORE_H
