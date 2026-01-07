#include "TermToTermQosManager.h"
#include "EndToEndQos.h"
#include <vector>
#include "../NetCombTransfer/NetCombTransferApi.h"


using std::make_pair;
using std::vector;

extern void QoS_TermOnOrOff(DWORD dwCID, bool bCreate);
extern bool QoS_GetFeedbackPacketFromNetCombTransfer(DWORD dwTID, const shared_ptr<BYTE> &pBuffer, DWORD dwLength, long int &recvtime, long int &fb_send_time);
extern void QoS_RecvRemoteToSelfQosInfo(shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_qos_info);
extern void QoS_RecvSelfToRemoteQosInfo(shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info);

namespace Term2TermQos
{
    TermToTermQosManager::TermToTermQosManager(DWORD d_self_tid) :m_d_self_tid(d_self_tid)
    {
        pthread_mutex_init(&m_cs, nullptr);
        Init();
    }
    TermToTermQosManager::~TermToTermQosManager()
    {
        pthread_mutex_destroy(&m_cs);
    }

    void TermToTermQosManager::AddEndToEndQos(DWORD d_remote_tid)
    {
        MutexLockGuard gGuard(m_cs);
        

        auto iter = m_end_to_end_qos_map.find(d_remote_tid);
        if (iter != m_end_to_end_qos_map.end())
        {
            //此端到端qos实例已经存在
        }
        else
        {
            shared_ptr<EndToEndQos> p_end_to_end_qos = make_shared<EndToEndQos>(m_d_self_tid, d_remote_tid);
            m_end_to_end_qos_map.insert(make_pair(d_remote_tid, p_end_to_end_qos));
        }
    }

    void TermToTermQosManager::RemoveEndToEndQos(DWORD d_remote_tid)
    {
        MutexLockGuard gGuard(m_cs);
        

        auto iter = m_end_to_end_qos_map.find(d_remote_tid);
        if (iter == m_end_to_end_qos_map.end())
        {
            //此端到端qos实例已经不存在
        }
        else
        {
            m_end_to_end_qos_map.erase(d_remote_tid);
        }
    }

    void TermToTermQosManager::RecvRemoteToSelfQosInfo(const shared_ptr<const RemoteToSelfQosInfo>& p_remote_to_self_qos_info)
    {
        MutexLockGuard gGuard(m_cs);
        

        auto iter = m_end_to_end_qos_map.find(p_remote_to_self_qos_info->m_d_remote_tid);
        if (iter == m_end_to_end_qos_map.end())
        {
            //没有这个qos映射，相关终端还没有被连接上
            //ut << "faailed" << endl;
        }
        else
        {
            shared_ptr<EndToEndQos> p_end_to_end_qos = iter->second;
            if (nullptr != p_end_to_end_qos)
            {
                if (p_remote_to_self_qos_info->m_d_datalength > 100) {
                    p_end_to_end_qos->RecvRemoteToSelfQosInfo(p_remote_to_self_qos_info);
                }
                //out << "p_remote_to_self_qos_info" << p_remote_to_self_qos_info->m_d_index << endl;
            }
        }
    }

    void TermToTermQosManager::RecvSelfToRemoteQosInfo(shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info)
    {
        pthread_mutex_lock(&m_cs);

        auto iter = m_end_to_end_qos_map.find(p_self_to_remote_qos_info->m_d_remote_tid);
        if (iter == m_end_to_end_qos_map.end())
        {
            //没有这个qos映射，相关终端还没有被连接上
            pthread_mutex_unlock(&m_cs);
        }
        else
        {
            shared_ptr<EndToEndQos> p_end_to_end_qos = iter->second;
            if (nullptr != p_end_to_end_qos)
            {
                pthread_mutex_unlock(&m_cs);
                p_end_to_end_qos->RecvSelfToRemoteQosInfo(p_self_to_remote_qos_info);
            }
            else
            {
                pthread_mutex_unlock(&m_cs);
            }
        }
    }

    void TermToTermQosManager::RecvRemote2SelfPacketQosFeedbackPacket(shared_ptr<const Remote2SelfPacketQosFeedbackPacket> p_feedback_packet)
    {
        MutexLockGuard gGuard(m_cs);
        
        //从反馈包实例中获取相应的包信息，并在map中找到对应的端到端实例
        auto iter = m_end_to_end_qos_map.find(p_feedback_packet->GetRemoteTermId());
        if (iter == m_end_to_end_qos_map.end())
        {
            ////std::cout << "failed" << std::endl;
            //没有这个qos映射，相关终端还没有被连接上
        }
        else
        {
            shared_ptr<EndToEndQos> p_end_to_end_qos = iter->second;
            if (nullptr != p_end_to_end_qos)
            {
                p_end_to_end_qos->RecvRemote2SelfPacketQosFeedbackPacket(p_feedback_packet);
                ////std::cout << "Remote2SelfPacketQosFeedbackPacket" << p_feedback_packet->GetLastNum() << std::endl;
            }
        }
    }
    TermToTermRoundTripTime TermToTermQosManager::GetTermToTermRTTInfo(const DWORD d_remote_tid) {
        //pthread_mutex_lock(&m_cs);
        MutexLockGuard gGuard(m_cs);
        TermToTermRoundTripTime res;
        res.roundtriptime = 5;
        auto iter = m_end_to_end_rtt_info.find(d_remote_tid);
        if (iter == m_end_to_end_rtt_info.end())
        {
            //没有这个qos映射，相关终端还没有被连接上
            return res;
            //pthread_mutex_unlock(&m_cs);
        }
        else
        {
            shared_ptr<TermToTermRoundTripTime> p_end_to_end_rtt = iter->second;
            if (nullptr != p_end_to_end_rtt)
            {
                return *p_end_to_end_rtt;
                //pthread_mutex_unlock(&m_cs);
            }
            else
            {
                return res;
                //pthread_mutex_unlock(&m_cs);
            }
        }
    }
    void TermToTermQosManager::RecvTermToTermRTTInfo(shared_ptr<TermRoundTripTimeInfo> TermtoTermrtt) {
        pthread_mutex_lock(&m_cs);
        auto iter = m_end_to_end_rtt_info.find(TermtoTermrtt->m_remote_tid);
        if (iter == m_end_to_end_rtt_info.end())
        {
            //shared_ptr<TermToTermRoundTripTime> p_end_to_end_rtt;
            m_end_to_end_rtt_info.insert(make_pair(TermtoTermrtt->m_remote_tid, make_shared<TermToTermRoundTripTime>(TermtoTermrtt->termtotermrtt)));
            //没有这个qos映射，相关终端还没有被连接上
            pthread_mutex_unlock(&m_cs);
        }
        else
        {
            shared_ptr<TermToTermRoundTripTime> p_end_to_end_rtt = iter->second;
            if (nullptr != p_end_to_end_rtt)
            {
                //m_end_to_end_rtt_info.insert(make_pair(TermtoTermrtt->m_remote_tid, make_shared<TermToTermRoundTripTime>(TermtoTermrtt->termtotermrtt)));
                iter->second = make_shared<TermToTermRoundTripTime>(TermtoTermrtt->termtotermrtt);
                //SPDLOG_LOGGER_WARN(termtotermqos_log, "rtt:, {},realrtt:, {}", p_end_to_end_rtt->roundtriptime, TermtoTermrtt->termtotermrtt.roundtriptime);
                pthread_mutex_unlock(&m_cs);
            }
            else
            {
                pthread_mutex_unlock(&m_cs);
            }
        }
    }
    void TermToTermQosManager::DealCycle()
    {
        std::map<DWORD, shared_ptr<EndToEndQos>>::iterator iter;
        pthread_mutex_lock(&m_cs);
        //将map复制到vector中
        vector<shared_ptr<EndToEndQos> > end_to_end_qos_vector;
        for (iter = m_end_to_end_qos_map.begin(); iter != m_end_to_end_qos_map.end(); iter++) {
            if (iter->second) {
                end_to_end_qos_vector.push_back(iter->second);
            }
        }
        pthread_mutex_unlock(&m_cs);
        ////std::cout << "end_to_end_qos_vector.size" << end_to_end_qos_vector.size() << std::endl;
        for (auto var : end_to_end_qos_vector)
        {
            var->DealCycle();
        }
    }

    void TermToTermQosManager::Init()
    {
        //注册到SpeedControl模块以获得数据输入
        Register_SpeedControl_RecvDataCallBack(QoS_GetFeedbackPacketFromNetCombTransfer); //注册回调函数，获得对端发送的反馈包
        RegisterSpeedControlTermOnOffCallBack(QoS_TermOnOrOff);							  //获得终端上线下线信息
        RegisterSpeedControlQoSSelf2RemoteInfoCallBack(QoS_RecvSelfToRemoteQosInfo);      //
        RegisterSpeedControlQoSRemote2SelfInfoCallBack(QoS_RecvRemoteToSelfQosInfo);      //
    }

}
