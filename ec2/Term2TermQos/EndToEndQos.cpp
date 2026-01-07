#include "EndToEndQos.h"

namespace Term2TermQos
{
    EndToEndQos::EndToEndQos(DWORD d_self_tid, DWORD d_remote_tid) : m_d_remote_tid(d_remote_tid), m_d_self_tid(d_self_tid)
    {
        m_remote_to_self_qos_unit = std::make_shared<Remote2SelfQosUnit>(m_d_self_tid, m_d_remote_tid);
        m_self_to_remote_qos_unit = std::make_shared<SelfToRemoteQosUnit>(m_d_self_tid, m_d_remote_tid);
    }
    void EndToEndQos::RecvRemoteToSelfQosInfo(shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_qos_info)
    {
        m_remote_to_self_qos_unit->RecvRemoteToSelfQosInfo(p_remote_to_self_qos_info);
    }
    void EndToEndQos::RecvSelfToRemoteQosInfo(shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info)
    {
        m_self_to_remote_qos_unit->RecvSelfToRemoteQosInfo(p_self_to_remote_qos_info);
    }
    void EndToEndQos::RecvRemote2SelfPacketQosFeedbackPacket(shared_ptr<const Remote2SelfPacketQosFeedbackPacket> p_feedback_packet)
    {
        m_self_to_remote_qos_unit->RecvRemote2SelfPacketQosFeedbackPacket(p_feedback_packet);
    }
    void EndToEndQos::DealCycle()
    {
        //printf("EndToEndQos::DealCycle\n");
        m_remote_to_self_qos_unit->DoCycleTime();
        m_self_to_remote_qos_unit->DoCycleTime();
    }
}
