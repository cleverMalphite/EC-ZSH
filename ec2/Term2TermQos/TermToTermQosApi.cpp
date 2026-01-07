/*#include "NetQoSMonitorApi.h"*/
#include "TermToTermQosManager.h"
#include "TermToTermQosApi.h"



//qos tx cid rtt table:  qos_tx_cid_rtt_sttable;
//qos tx cid loss table: qos_tx_cid_loss_sttable;

//qos tx tid rtt table:  qos_tx_tid_rtt_sttable;
//qos tx tid loss table: qos_tx_tid_loss_sttable;


std::shared_ptr<Term2TermQos::TermToTermQosManager> g_term_to_term_qos_manager = nullptr;
bool g_b_qos_stop_thread = false;	//为true时表示本模块自带线程不应该退出
using Term2TermQos::TermToTermQosManager;
namespace Term2TermQos {
    const static DWORD QOS_MAX_REGISTER_DATA_FUNC = 10;
    pMRUDPGetQosRTTInfoCallBack g_p_data_func[QOS_MAX_REGISTER_DATA_FUNC];
    pRttCallBackSimple g_qos_rtt_callback = NULL;
}
using Term2TermQos::g_qos_rtt_callback;
//线程函数等辅助函数
namespace {
    pthread_t QosCycleThreadID_;
    pthread_cond_t QosCycleCond_;

    void *TermToTermQosCycleFunc(void *argNeed) {
//        printf("QOSThread Open\n");
        while (!g_b_qos_stop_thread) {
            if (nullptr != g_term_to_term_qos_manager) {
                g_term_to_term_qos_manager->DealCycle();
            }
        }
        return nullptr;
    }
}


namespace Qos{
}

bool InitTermToTermQos(const DWORD d_self_tid) {
    g_term_to_term_qos_manager = std::make_shared<TermToTermQosManager>(d_self_tid);
    g_b_qos_stop_thread = false;
    sleep(1);
    pthread_create(&QosCycleThreadID_, nullptr, &TermToTermQosCycleFunc, nullptr);
    return true;
}

bool UninitTermToTermQos() {
    g_b_qos_stop_thread = true;
    pthread_join(QosCycleThreadID_, nullptr);
    g_term_to_term_qos_manager = nullptr;
    return true;
}

bool RegisterRttSimpleCallBack(pRttCallBackSimple qos_rtt_callback) {
    g_qos_rtt_callback = qos_rtt_callback;
    return true;
}
TermToTermRoundTripTime MRUDP_Get_QosRtt(const DWORD d_remote_tid) {
    if (nullptr != g_term_to_term_qos_manager)
    {
        return g_term_to_term_qos_manager->GetTermToTermRTTInfo(d_remote_tid);
    }
}
bool RateInfo(DWORD m_remote_tid, DWORD m_d_speed)
{
    DWORD remote_tid, d_speed;
    remote_tid = m_remote_tid;
    d_speed = m_d_speed;
    return true;
}

void QoSRoundTripTimeInfo(shared_ptr<TermRoundTripTimeInfo> term_rtt_info) {
    if (nullptr != g_term_to_term_qos_manager)
    {
        g_term_to_term_qos_manager->RecvTermToTermRTTInfo(term_rtt_info);
    }
}
//注册到NetCombTransfer模块的终端上下线回调函数
void QoS_TermOnOrOff(DWORD dwCID, bool bCreate)
{
    if (nullptr != g_term_to_term_qos_manager)
    {
        if (bCreate)
        {
            g_term_to_term_qos_manager->AddEndToEndQos(static_cast<DWORD>(dwCID));
        }
        else
        {
            g_term_to_term_qos_manager->RemoveEndToEndQos(dwCID);
        }
    }
}

//注册到SpeedControl模块获得远端反馈包的函数
bool QoS_GetFeedbackPacketFromNetCombTransfer(DWORD dwTID, const shared_ptr<BYTE> &pBuffer, DWORD dwLength, long int&recvtime, long int&fb_send_time)
{
    using Term2TermQos::Remote2SelfPacketQosFeedbackPacket;

    if (Remote2SelfPacketQosFeedbackPacket::IsInstanceOf(pBuffer, dwLength))
    {
        //如果是Remote2SelfPacketQosFeedbackPacket类型的反馈包，那么对其解码
        std::shared_ptr<Remote2SelfPacketQosFeedbackPacket> p_feedback_packet = std::make_shared<Remote2SelfPacketQosFeedbackPacket>();
        p_feedback_packet->Decode(pBuffer, dwLength);
        //printf("[QOSDebug]::Recv A FeedBackPacket , Decode get last_recv_timestamp is %ld\n",p_feedback_packet->GetLastNumSendTime());
        p_feedback_packet->SetRecvTime(recvtime);
        p_feedback_packet->SetFb_SendTime(fb_send_time);
        if (nullptr != g_term_to_term_qos_manager)
        {
            g_term_to_term_qos_manager->RecvRemote2SelfPacketQosFeedbackPacket(p_feedback_packet);
        }
    }
    return true;
}

void QoS_RecvRemoteToSelfQosInfo(shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_qos_info)
{
    if (nullptr != g_term_to_term_qos_manager)
    {
        g_term_to_term_qos_manager->RecvRemoteToSelfQosInfo(p_remote_to_self_qos_info);
    }
}

void QoS_RecvSelfToRemoteQosInfo(shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info)
{
    if (nullptr != g_term_to_term_qos_manager)
    {
        g_term_to_term_qos_manager->RecvSelfToRemoteQosInfo(p_self_to_remote_qos_info);
    }
}
void  Test_TermOnOrOff(DWORD dwCID, bool bCreate) {
    if (nullptr != g_term_to_term_qos_manager)
    {
        if (bCreate)
        {
            g_term_to_term_qos_manager->AddEndToEndQos(static_cast<DWORD>(dwCID));
        }
        else
        {
            g_term_to_term_qos_manager->RemoveEndToEndQos(dwCID);
        }
    }
}
bool Test_GetFeedbackPacketFromNetCombTransfer(DWORD dwTID, shared_ptr<BYTE> pBuffer, DWORD dwLength, bool bMsg) {
    using Term2TermQos::Remote2SelfPacketQosFeedbackPacket;

    if (Remote2SelfPacketQosFeedbackPacket::IsInstanceOf(pBuffer, dwLength))
    {
        //如果是Remote2SelfPacketQosFeedbackPacket类型的反馈包，那么对其解码
        std::shared_ptr<Remote2SelfPacketQosFeedbackPacket> p_feedback_packet = std::make_shared<Remote2SelfPacketQosFeedbackPacket>();
        p_feedback_packet->Decode(pBuffer, dwLength);
        ////std::cout << "decode success" << std::endl;
        ////std::cout << "Remote2SelfPacketQosFeedbackPacket" << p_feedback_packet ->GetLastNum()<< std::endl;
        if (nullptr != g_term_to_term_qos_manager)
        {
            ////std::cout << "entered" << std::endl;
            g_term_to_term_qos_manager->RecvRemote2SelfPacketQosFeedbackPacket(p_feedback_packet);
        }
    }
    return true;
}
void  Test_RecvRemoteToSelfQosInfo(shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_qos_info) {
    if (nullptr != g_term_to_term_qos_manager)
    {
        g_term_to_term_qos_manager->RecvRemoteToSelfQosInfo(p_remote_to_self_qos_info);
        ////std::cout << "p_remote_to_self_qos_info" << p_remote_to_self_qos_info->m_d_index << std::endl;
    }
}

void Test_RecvSelfToRemoteQosInfo(shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info)
{
    if (nullptr != g_term_to_term_qos_manager)
    {
        g_term_to_term_qos_manager->RecvSelfToRemoteQosInfo(p_self_to_remote_qos_info);
    }
}
//TermSpeedInfo Test_getspeedinfo() {
//	return speedinfo.pop();
//}
