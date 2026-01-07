//
// Created by 王炳棋 on 2022/12/28.
//
#include "ReliableTransferLog.h"
#include <iostream>

namespace MRUDP {

    void ReliableTransferLog::AddSimpleSendNumber() {
        m_d_simple_send_number++;
        if (0 == (m_d_simple_send_number % Reliable_TransferLog_Every_This_Number)) {
            std::cout << "[simple_send_number]:" << m_d_simple_send_number << ", [resend_number]:" << m_d_resend_number << std::endl;
            m_d_resend_number = 0;
            m_d_simple_send_number = 0;
        }
    }

    void ReliableTransferLog::GetLog(DWORD &simple_send_number, DWORD &resend_number) const {
        simple_send_number = m_d_simple_send_number;
        resend_number = m_d_resend_number;
    }
}