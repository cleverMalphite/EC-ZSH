#pragma once

#include <map>
#include <memory>
#include "EndToEndQos.h"
#include "QosStructInterfaceInfo.h"
#include "../SpeedControl/SpeedControlApi.h"
#include "Remote2SelfPacketQosFeedbackPacket.h"
#include "../Util/LockGuard.h"

using std::map;
using std::shared_ptr;

namespace Term2TermQos
{
    class TermToTermQosManager
    {
    public:
        TermToTermQosManager(DWORD d_self_tid);
        virtual ~TermToTermQosManager();
        //添加与id号为d_remote_tid的终端之间的 端到端Qos实例
        void AddEndToEndQos(DWORD d_remote_tid);
        //删除与id号为d_remote_tid的终端之间的 端到端Qos实例
        void RemoveEndToEndQos(DWORD d_remote_tid);

        void RecvRemoteToSelfQosInfo(const shared_ptr<const RemoteToSelfQosInfo>& p_remote_to_self_qos_info);
        void RecvSelfToRemoteQosInfo(shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info);
        TermToTermRoundTripTime GetTermToTermRTTInfo(const DWORD d_remote_tid);
        void RecvRemote2SelfPacketQosFeedbackPacket(shared_ptr<const Remote2SelfPacketQosFeedbackPacket> p_feedback_packet);
        void RecvTermToTermRTTInfo(shared_ptr<TermRoundTripTimeInfo> termtotermrtt);
        void DealCycle();	//会被周期性调用的函数
    private:
        pthread_mutex_t m_cs;	//临界区对象，用于同步，主要是关于EndToEndQos实例map映射的同步
        map<DWORD, shared_ptr<EndToEndQos> > m_end_to_end_qos_map;	//对端终端号到端到端Qos实例的映射
        map <DWORD, shared_ptr<TermToTermRoundTripTime> >m_end_to_end_rtt_info;
        const DWORD m_d_self_tid;	//本端终端号

        void Init();
    };
}
