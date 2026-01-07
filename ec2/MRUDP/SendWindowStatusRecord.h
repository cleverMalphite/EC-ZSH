//
// Created by 王炳棋 on 2022/12/31.
//

#ifndef NETCOMBTRANSFER_SENDWINDOWSTATUSRECORD_H
#define NETCOMBTRANSFER_SENDWINDOWSTATUSRECORD_H

#include "SerializeUtil.h"
#include "Store_H/ReliableUdpDataReceiverStore.h"


namespace MRUDP {
    /**
     * 用来存储端到端传输实体发送窗口确认状态链表中的一个元素
     * 显然，这个数据应该根据每次确认的尾包进行更改
     */
    class SendWindowStatusRecord {
    private:
        DWORD m_cumulative_packet_index = 0;    //当前发送窗口中得到累积确认的最后一个数据包索引
        DWORD m_cumulative_create_timestamp = 0;    //m_cumulative_packet_index对应的数据包生成时候的时间
        BYTE m_priority = 0;    //对应尾包的包传输优先级
        DWORD m_cumulative_timestamp = 0;    //对应的尾包得到累积确认时的时间戳
        DWORD m_sacked_timestamp = 0;    //对应的尾包第一次被接收时的时间戳

        BYTE m_request_retransfer_time = 0;    //表示发送端自上次累积确认起，因为请求重传机制重传数据包的数目
        BYTE m_timeout_retransfer_time = 0;    //表示发送端自上次累积确认起，因为超时重传机制重传数据包的数目

        DWORD m_cumulative_acked_packet_num = 0;    //表示此次一共累积确认了多少数据包

    public:
        void AddRequestReTransferTime() {
            m_request_retransfer_time++;
        }

        void AddTimeoutRetransferTime() {
            m_timeout_retransfer_time;
        }

        DWORD GetRequestReTransferTime() const {
            return m_request_retransfer_time;
        }

        DWORD GetTimeoutReTransferTime() const {
            return m_timeout_retransfer_time;
        }

        void SetCumulativeAckedPacketNumber(DWORD acked_number) {
            m_cumulative_acked_packet_num = acked_number;
        }

        void SetNewCumulativePacket(ReliableUdpDataReceiverStore &data);    //根据一个新累积确认的尾包来更新这一变量的值
    };
}
#endif //NETCOMBTRANSFER_SENDWINDOWSTATUSRECORD_H
