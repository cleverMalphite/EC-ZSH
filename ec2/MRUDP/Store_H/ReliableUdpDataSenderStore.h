//
// Created by 王炳棋 on 2022/12/21.
//

#include <utility>

#include "Transfer_H/ReliableUdpData.h"

#ifndef NETCOMBTRANSFER_RELIABLEUDPDATASENDERSTORE_H
#define NETCOMBTRANSFER_RELIABLEUDPDATASENDERSTORE_H

namespace MRUDP {

    /*
    * 这个类供数据发送端来存取发送窗口里的数据
    */
    class ReliableUdpDataSenderStore :
            public ReliableUdpData {
    private:
        BYTE m_priority = 0;                 //包重传优先级
        DWORD m_data_bytes_length = 0;       //编码成功后数据字节序列的长度（单位为字节）
        BYTE m_retransfer_time = 1;          //重传次数
        DWORD m_last_retransfer_time = 0;    //上次进行传输的时间戳
    public:
        explicit ReliableUdpDataSenderStore(std::shared_ptr<BYTE> data, const DWORD dLength) : ReliableUdpData(std::move(data), dLength) {
            m_last_retransfer_time = this->m_udTimeStamp;
        }

        explicit ReliableUdpDataSenderStore() : ReliableUdpData(nullptr, 0) {

        }

        bool SetEncodeBytes(DWORD bytes_length) {
            m_data_bytes_length = bytes_length;
            return true;
        }

        bool GetTransferPriority() const {
            return m_priority;
        }

        BYTE GetReTransferTimeThreshold() const {
            return m_retransfer_time;
        }

        void ChangeReTransferTimeThreshold() {
            m_retransfer_time += 1;
            if (m_retransfer_time > 10) {
            }
        }

        DWORD GetLastReTransferTime() const {
            return m_last_retransfer_time;
        }

        void SetLastReTransferTime(DWORD last_retransfer_time) {
            m_last_retransfer_time = last_retransfer_time;
        }
    };
}

#endif //NETCOMBTRANSFER_RELIABLEUDPDATASENDERSTORE_H
