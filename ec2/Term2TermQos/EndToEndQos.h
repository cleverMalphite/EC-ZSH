#pragma once
#include <memory>
#include <map>
#include "Remote2SelfQosUnit.h"
#include "SelfToRemoteQosUnit.h"
#include "bbr_controller.h"
using std::shared_ptr;
using std::map;
using std::make_shared;

namespace Term2TermQos
{
    /**
     * 这个类代表着一个端到端互为发送接收端的实体，本端相对于对端，既是发送端，又是接收端
     */
    class EndToEndQos
    {
    public:
        EndToEndQos(DWORD d_self_tid, DWORD d_remote_tid);
        virtual ~EndToEndQos() {};

        void RecvRemoteToSelfQosInfo(shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_qos_info);
        void RecvSelfToRemoteQosInfo(shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info);
        void RecvRemote2SelfPacketQosFeedbackPacket(shared_ptr<const Remote2SelfPacketQosFeedbackPacket> p_feedback_packet);
        void DealCycle();	//会被周期性调用的函数
    private:
        const DWORD m_d_self_tid;	//本端终端号
        const DWORD m_d_remote_tid;	//对端终端号

        shared_ptr<Remote2SelfQosUnit> m_remote_to_self_qos_unit;	//此处理单元用于给本端速率控制模块发送反馈
        shared_ptr<SelfToRemoteQosUnit> m_self_to_remote_qos_unit;	//此处理单元用于给对端发送QOS反馈包
    };
}
