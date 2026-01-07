//
// Created by 王炳棋 on 2022/12/31.
//

#include "SendWindowStatusRecord.h"

void MRUDP::SendWindowStatusRecord::SetNewCumulativePacket(ReliableUdpDataReceiverStore &data) {
    m_cumulative_acked_packet_num = data.GetPacketIndex();
    m_cumulative_create_timestamp = data.GetTimeStamp();
    m_priority = data.GetPriority();
    m_cumulative_create_timestamp = data.GetCumulativeTimeStamp();
    m_sacked_timestamp = data.GetSackedTimeStamp();
}
