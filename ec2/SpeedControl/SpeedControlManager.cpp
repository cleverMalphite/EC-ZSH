//
// Created by 王炳棋 on 2022/12/10.
//

#include "SpeedControlManager.h"

#include <utility>
#include "../NetCombTransfer/NetCombTransferApi.h"
#include "DataWithPacketInfo.h"

/**
 * 下面是一些调用全局回调函数的全局方法
 */
//调用所有的数据回调函数
extern void
CallAllDataFunc(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dwLength, int64_t &recvtime,
                int64_t &fb_send_time, bool isRBUDP);

//调用QoS注册的两个回调函数的全局函数
extern void CallQoSSelf2RemoteInfoFunc(std::shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info);

extern void CallQoSRemote2SelfInfoFunc(std::shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_qos_info);

//注册到NetCombTransfer模块的路由回调
extern void
ON_ROUTE_CALL_BACK_FROM_SPEEDCONTROL(DWORD dwSourceTID, DWORD dwDestinationTID, DWORD m_nNop, DWORD m_nQoSSend,
                                     DWORD m_nQosRecv, bool bSetted);

//本模块注册到QoS模块的数据发送速率控制信息回调函数
void SpeedControl_QoSTermOnOffCallBack(DWORD dwTID, bool bCreate);

void SpeedControl_RBUDPIndexCallBack(DWORD dwTID, DWORD dIndex);

namespace SpeedControl {

    extern std::shared_ptr<SpeedControlManager> gSpeedControlManager;

    extern bool g_speed_control_stop;

    extern bool 
    SpeedControl_NetCombTransferMessageCallback(DWORD dwCID, bool bCreate);

    extern bool
    SpeedControl_NetCombTransferBufferCallback(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dwLength,
                                               bool bMsg,
                                               int64_t Packet_Recv_Timestamp);


    SpeedControlManager::SpeedControlManager(DWORD d_self_tid) : m_self_tid(d_self_tid) {
        pthread_mutex_init(&Mutex_, nullptr);
    }

    SpeedControlManager::~SpeedControlManager() {
        Uninit();
        pthread_mutex_destroy(&Mutex_);
    }

    /*speedcontrol工作线程*/
    void *SpeedControlPlayThreadProc(void *argNeeded) {
        //printf("[SpeedControl]::工作线程开启\n");
        bool bResult = false;
        if (nullptr != gSpeedControlManager) {
            while (!g_speed_control_stop) {
                bResult = gSpeedControlManager->DoCycle();
                if (!bResult) {
                    //Mark:这个休眠方式需要修改，可以尝试使用pthread_cond_t
                    usleep(1000);
                    //sleep(1);
                }
            }
        }
        //gSpeedControlManager->Uninit();
        return nullptr;
    }

    bool SpeedControlManager::Init() {
        //开始向NetCombTransfer模块和QoS模块注册数据
        Register_NetCombTransferCallback(SpeedControl_NetCombTransferMessageCallback,
                                         SpeedControl_NetCombTransferBufferCallback);
        Register_NetCombTransferRouterCallback(ON_ROUTE_CALL_BACK_FROM_SPEEDCONTROL);
        //SpeedControl工作线程开启
        bool isErr = pthread_create(&SpeedControlPID, nullptr, SpeedControlPlayThreadProc, nullptr);
        if (isErr != 0) {
            return false;
        }
        return true;
    }

    bool SpeedControlManager::Uninit() {
        pthread_join(SpeedControlPID, nullptr);
        return true;
    }

    bool SpeedControlManager::DoCycle() {
        bool bResult = false;
        bResult = DoTermOnOrOff() || bResult;
        bResult = DoRealCycleSend() || bResult;
        return bResult;
    }
    bool Isprint=true;
    void SpeedControlManager::DoRecvData(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dLength,
                                         int64_t &recvtime) {

        //TEST:测试视频传输
//        fwrite(pBuffer.get(),dLength,1,fp_DataWithPacketInfo_recv);

        //printf("<SpeedControlManager>DoRecvData dlength is %d\n",dLength);
        std::shared_ptr<DataWithPacketInfo> p_data_packet = std::make_shared<DataWithPacketInfo>(dwTID);
        p_data_packet->Decode(pBuffer, dLength);
        p_data_packet->GetRemoteToSelfQoSInfo()->recv_timestamp = recvtime;
        //printf("[QOSDebug]::SpeedControl DoRecvData : R2S Info, Recv TimeStamp is %ld\n",recvtime);
        CallQoSRemote2SelfInfoFunc(p_data_packet->GetRemoteToSelfQoSInfo());
        //printf("CallQoSRemote2SelfInfoFunc End\n");
        int64_t fd_send = p_data_packet->GetRemoteToSelfQoSInfo()->m_d_timestamp;
        bool isRBUDP = p_data_packet->GetWhichFunc();
        if(Isprint){
            printf("current send func is %d\n",isRBUDP);
            Isprint=false;
        }

        //printf("[QOSDebug]::SpeedControl DoRecvData : R2S Info, fd_send time is %ld\n",recvtime);
        CallAllDataFunc(p_data_packet->GetRemoteTID(), p_data_packet->GetData(), p_data_packet->GetDataLength(),
                        recvtime, fd_send, isRBUDP);
        //printf("CallAllDataFunc (MRUDP + QOS) End, DataLength is %d\n",p_data_packet->GetDataLength());

        /*int64_t fd_send = 0;
        CallAllDataFunc(dwTID, pBuffer, dLength, recvtime, fd_send);*/
    }

    DWORD64 SendPacketNumber = 0;

    void SpeedControlManager::DoRecvTermOnOrOff(DWORD dwTID, bool bCreate) {
        std::shared_ptr<TermOnOffInfo> term_info = std::make_shared<TermOnOffInfo>();
        term_info->m_remote_tid = dwTID;
        term_info->m_b_on = bCreate;
        pthread_mutex_lock(&Mutex_);
        m_term_on_or_off_info.push(term_info);
        pthread_mutex_unlock(&Mutex_);
        //printf("[Test]::SendNum is %lu\n",SendPacketNumber);
        //通知QoS模块终端上下线
        SendPacketNumber = 0;
        SpeedControl_QoSTermOnOffCallBack(dwTID, bCreate);
    }

    void SpeedControlManager::DoRecvSpeedInfo(TermSpeedInfo term_speed_info) {
        std::shared_ptr<Term2TermTransmission> p_t2t = nullptr;
        pthread_mutex_lock(&Mutex_);
        auto iter = m_map_term_to_term_transmission.find(term_speed_info.m_remote_tid);
        if (iter != m_map_term_to_term_transmission.end()) {
            p_t2t = iter->second;
        }
        pthread_mutex_unlock(&Mutex_);
        if (p_t2t) {
            std::shared_ptr<TermSpeedInfo> p_t2t_speed = std::make_shared<TermSpeedInfo>();
            p_t2t_speed->m_d_speed = term_speed_info.m_d_speed;
            p_t2t_speed->m_remote_tid = term_speed_info.m_remote_tid;
            p_t2t->RecvTermSpeedInfo(p_t2t_speed);
        }
    }


    bool SpeedControlManager::DoSendData(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dLength, bool isSleep,
                                         bool isRBUDP) {
        pthread_mutex_lock(&Mutex_);
        SendPacketNumber++;
        static std::shared_ptr<Term2TermTransmission> p_last_term_2_term = nullptr;
        std::shared_ptr<Term2TermTransmission> p_temp_term_2_term = nullptr;
        //printf("useCount:%ld\n",p_last_term_2_term.use_count());
        /*if (p_last_term_2_term.use_count() < 2) {
            static std::shared_ptr<Term2TermTransmission> p_last_term_2_term = nullptr;
        }*/
        if (/*p_last_term_2_term*//*p_last_term_2_term.use_count() < 0*/
                !ChannelUpdateFlagForSendData && p_last_term_2_term != nullptr
                && dwTID == p_last_term_2_term->GetDestTID()) {
            //错误，不能得出上一个传输通道实例就是p_last_term_2_term的结论。
            //设想，当传输实例关闭后，p_last_term_2_term仍然有可能（dwTID等于其终端号时）保存上一次的传输通道实例，则将数据包放错了地方。
            //printf("[SC::AutoTestDebug]::仍然使用上一次的传输通道实例\n");
            p_temp_term_2_term = p_last_term_2_term;
            pthread_mutex_unlock(&Mutex_);
        } else {
            //printf("\n[SC::AutoTestDebug]::更新上一次的传输通道实例\n\n");
            auto iter = m_map_term_to_term_transmission.find(dwTID);
            if (iter != m_map_term_to_term_transmission.end()) {
                p_temp_term_2_term = iter->second;
                p_last_term_2_term = p_temp_term_2_term;
                ChannelUpdateFlagForSendData = false;
            }
            pthread_mutex_unlock(&Mutex_);
        }
        if (p_temp_term_2_term) {
            //printf("[SC::AutoTestDebug]::找到了端到端通信实体，pushSendData\n");
           // printf("[debug1208:SC_do_send_data1 false]\n");
            return p_temp_term_2_term->PushSendData(pBuffer, dLength, isSleep, isRBUDP);
        }
       // printf("[debug1208:SC_do_send_data2 false]\n");
        return false;
    }

    bool
    SpeedControlManager::DoSendDataPri(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dLength, bool isSleep,
                                       bool isRBUDP) {
        pthread_mutex_lock(&Mutex_);
        SendPacketNumber++;
        static std::shared_ptr<Term2TermTransmission> p_last_term_2_term = nullptr;
        std::shared_ptr<Term2TermTransmission> p_temp_term_2_term = nullptr;
        if (!ChannelUpdateFlagForSendDataPri && p_last_term_2_term != nullptr &&
            dwTID == p_last_term_2_term->GetDestTID()) {
            p_temp_term_2_term = p_last_term_2_term;
            pthread_mutex_unlock(&Mutex_);
        } else {
            auto iter = m_map_term_to_term_transmission.find(dwTID);
            if (iter != m_map_term_to_term_transmission.end()) {
                p_temp_term_2_term = iter->second;
                p_last_term_2_term = p_temp_term_2_term;
                ChannelUpdateFlagForSendDataPri = false;
            }
            pthread_mutex_unlock(&Mutex_);
        }

        if (p_temp_term_2_term) {
            return p_temp_term_2_term->PushSendDataPri(pBuffer, dLength, isSleep, isRBUDP);
        }
        return false;
    }

    bool
    SpeedControlManager::DoSendDataWithoutSpeedControl(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dLength,
                                                       bool isRBUDP) {
        /*int64_t oldts = su_get_sys_time_tmp() / 1000;*/
        pthread_mutex_lock(&Mutex_);
        static std::shared_ptr<Term2TermTransmission> p_last_term_2_term = nullptr;
        std::shared_ptr<Term2TermTransmission> p_temp_term_2_term = nullptr;
        if (!ChannelUpdateFlagForSendDataWithoutSC && p_last_term_2_term != nullptr &&
            dwTID == p_last_term_2_term->GetDestTID()) {
            p_temp_term_2_term = p_last_term_2_term;
            pthread_mutex_unlock(&Mutex_);
        } else {
            auto iter = m_map_term_to_term_transmission.find(dwTID);
            if (iter != m_map_term_to_term_transmission.end()) {
                p_temp_term_2_term = iter->second;
                p_last_term_2_term = p_temp_term_2_term;
                ChannelUpdateFlagForSendDataWithoutSC = false;
            }
            pthread_mutex_unlock(&Mutex_);
        }

        if (p_temp_term_2_term) {
            return p_temp_term_2_term->DoSendDataWithoutSpeedControl(pBuffer, dLength, isRBUDP);
        }
        return false;
    }

    bool SpeedControlManager::SendTIDCommandWithoutSpeedControl(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer,
                                                                DWORD dLength, bool isRBUDP) {

        std::shared_ptr<Term2TermTransmission> p_temp_term_2_term = nullptr;
        bool bResult = false;
        pthread_mutex_lock(&Mutex_);
        auto iter = m_map_term_to_term_transmission.find(dwTID);
        if (iter != m_map_term_to_term_transmission.end()) {
            p_temp_term_2_term = iter->second;
        }
        pthread_mutex_unlock(&Mutex_);
        if (p_temp_term_2_term) {
            //printf("[SpeedTestDebug]::向%d发送command,不加速率控制\n", dwTID);
            return p_temp_term_2_term->SendTIDCommandWithoutSpeedControl(pBuffer, dLength, isRBUDP);
        }
        return false;
    }

    bool SpeedControlManager::DoTermOnOrOff() {
        bool bResult = false;
        pthread_mutex_lock(&Mutex_);
        std::shared_ptr<TermOnOffInfo> term_on_off_info = nullptr;
        while (!m_term_on_or_off_info.empty()) {
            bResult = true;
            term_on_off_info = m_term_on_or_off_info.front();
            if (!term_on_off_info->m_b_on) {
                m_map_term_to_term_transmission.erase(term_on_off_info->m_remote_tid);
                //printf("[AutoTestDebug]::终端%d下线！\n", term_on_off_info->m_remote_tid);
            } else {
                if (m_map_term_to_term_transmission.end() ==
                    m_map_term_to_term_transmission.find(term_on_off_info->m_remote_tid)) {
                    std::shared_ptr<Term2TermTransmission> p_t2t =
                            std::make_shared<Term2TermTransmission>(term_on_off_info->m_remote_tid,
                                                                    GetIntegerKeyIni("Main",
                                                                                     "SPEEDCONTROL_MAX_UNDEAL_NUMBER",
                                                                                     2000));
                    m_map_term_to_term_transmission.insert(
                            std::pair<DWORD, std::shared_ptr<Term2TermTransmission>>(term_on_off_info->m_remote_tid,
                                                                                     p_t2t));
                    //("[AutoTestDebug]::终端%d上线！\n", term_on_off_info->m_remote_tid);
                }
            }
            m_term_on_or_off_info.pop();
        }
        ChannelUpdateFlagForSendData = true;
        ChannelUpdateFlagForSendDataPri = true;
        ChannelUpdateFlagForSendDataWithoutSC = true;
        pthread_mutex_unlock(&Mutex_);
        return bResult;
    }

    bool SpeedControlManager::DoRealCycleSend() {
        bool bResult = false;
        pthread_mutex_lock(&Mutex_);
        m_map_term_to_term_transmission_tmp = m_map_term_to_term_transmission;
        pthread_mutex_unlock(&Mutex_);
        /*auto iter = m_map_term_to_term_transmission_tmp.begin();
        while (iter != m_map_term_to_term_transmission_tmp.begin())*/
        for (const auto &p_temp_t2t: m_map_term_to_term_transmission_tmp) {
            bResult = bResult || p_temp_t2t.second->DoSendData();
            //printf("终端数目:%lu,DoRealCycleSend return value is %d\n", m_map_term_to_term_transmission_tmp.size(), bResult);
        }
        return bResult;
    }

    std::shared_ptr<Term2TermTransmission> SpeedControlManager::GetTerm2TermTransMission(DWORD nTID) {
        std::shared_ptr<Term2TermTransmission> p_temp_term_2_term = nullptr;
        bool bResult = false;
        pthread_mutex_lock(&Mutex_);
        auto iter = m_map_term_to_term_transmission.find(nTID);
        if (iter != m_map_term_to_term_transmission.end()) {
            p_temp_term_2_term = iter->second;
        } else {
            pthread_mutex_unlock(&Mutex_);
            return nullptr;
        }
        pthread_mutex_unlock(&Mutex_);
        return p_temp_term_2_term;
    }
}

