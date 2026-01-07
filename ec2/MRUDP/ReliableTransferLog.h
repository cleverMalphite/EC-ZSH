//
// Created by 王炳棋 on 2022/12/28.
//

#include "SerializeUtil.h"

#ifndef NETCOMBTRANSFER_RELIABLETRANSFERLOG_H
#define NETCOMBTRANSFER_RELIABLETRANSFERLOG_H
namespace MRUDP {
    class ReliableTransferLog {
    public:
        ReliableTransferLog() : m_d_simple_send_number(0), m_d_resend_number(0) {

        }

    private:
        DWORD m_d_simple_send_number;    //本端预期发送的包数目
        DWORD m_d_resend_number;        //本端重发的包数目
        static const DWORD Reliable_TransferLog_Every_This_Number = 1000;    //每发送这个数目的包就记录一次重发率
    public:
        void AddSimpleSendNumber();    //本端发送的包数目加一

        //将本端重发的包数目加一
        void AddReSendNumber(DWORD dNumber) {
            m_d_resend_number += dNumber;
        };

        void GetLog(DWORD &simple_send_number, DWORD &resend_number) const;    //通过参数获取本端发送的包数目和重发的包数目
    };

}


#endif //NETCOMBTRANSFER_RELIABLETRANSFERLOG_H
