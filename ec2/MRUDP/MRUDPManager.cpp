//
// Created by 王炳棋 on 2022/12/30.
//
#include "MRUDPManager.h"
#include "../NetCombTransfer/NetCombTransferApi.h"
#include "../SpeedControl/QosStructInterfaceInfo.h"
#include "../SpeedControl/SpeedControlApi.h"
#include <queue>
#include <memory>
#include <utility>
#include <iostream>
#include "Transfer_H/ReliableForceSpeedMessage.h"
#include "Transfer_H/ReliableForceAdjustBandWidthMessage.h"

#include "../Util/SystemTimeFunc.h"
#include "../Util/VideoRtp_callback.h"

#include <thread>

#define DEBUG_MRUDPMANAGER

namespace MRUDP {
    extern std::shared_ptr<MRUDPManager> gMRUDPManager;
    //extern DWORD selftermID_for_test;

    void MRUDPManager::DoCycle() {
        //printf("[MRUDP]::工作线程开启\n");
        const int MAX_DEAL_DATA_ONE_TIME = 1000;
        bool NeedSleep = true;
        int i = 0;
        while (undeal_data_thread_flag.test_and_set()) {
            DealMessageWork() ? NeedSleep = false : NeedSleep = true;
            i = 0;
            while (i < MAX_DEAL_DATA_ONE_TIME && DealDataWork()) {
                NeedSleep = false;
                i++;
            }
            if (NeedSleep) {
                //printf("[MRUDPDebug]::Data Processing undealDataNum is %ld\n",m_undealData.size());
                //std::this_thread::sleep_for(std::chrono::microseconds(/*50*/1000));
                usleep(1000);
            }
        }
        //printf("[MRUDP]::工作线程终止\n");
    }

    void ON_ROUTE_CALL_BACK_FROM_MRUDP(
            DWORD dwSourceTID, DWORD dwDestinationTID, 
            UINT m_nNop, UINT m_nQoSSend,
            UINT m_nQosRecv,
            bool bSetted) 
    {
        //获取本终端ID
        if (bSetted) {
            //printf("[MRUDPTransferTest]::RouterCallBackFromNetCombTransfer,SetUp New End2EndReliableTransmission\n");
            static const DWORD dSelfTermId = GetIntegerKeyIni("Main", "DeviceID", 0);
            gMRUDPManager->InitSingleEnd2EndReliableTransmission(
                    std::make_shared<MRUDPTerm2TermConfig>(dwDestinationTID, dSelfTermId));
        } else {
            //printf("[MRUDPTransferTest]::RouterCallBackFromNetCombTransfer,ShutDown an End2EndReliableTransmission\n");
            gMRUDPManager->RemoveSingleEnd2EndReliableTransmission(dwDestinationTID);
        }
    }

    //统计MRUDP收到数据包间隔
    DWORD Recv_Count = 0;  //总收包次数
    DWORD OutTime_count = 0;  //超时次数
    DWORD Last_time = 0;
    DWORD This_time = 0;

    DWORD recv_data_count2 = 0;
    DWORD number_of_thousand2 = 0;

    bool Isprint=true;
    bool
    ON_RECV_DATA_FROM_NETCOMBTRANSFER(DWORD dwTID, const shared_ptr<BYTE> &pBuffer, DWORD dwLength, int64_t &recvtime,
                                      int64_t &fb_send_time) {
        if (dwLength <= 0) {
            return true;
        }
        const BYTE *raw = pBuffer.get();
        if (raw && dwLength >= 13 && raw[0] == 0x00 && ((raw[1] & 0xC0) == 0x80) && g_video_rtp_recv_callback) {
            static uint64_t s_fwd_pkts = 0;
            static uint64_t s_fwd_bytes = 0;
            static int64_t s_last_ms = 0;
            s_fwd_pkts += 1;
            s_fwd_bytes += static_cast<uint64_t>(dwLength);
            const int64_t now_ms = static_cast<int64_t>(GetTickCount());
            if (s_last_ms == 0) s_last_ms = now_ms;
            const int64_t dt = now_ms - s_last_ms;
            if (dt >= 1000) {
                const uint64_t pps = (s_fwd_pkts * 1000ULL) / static_cast<uint64_t>(dt);
                const uint64_t kbps = (s_fwd_bytes * 8ULL) / static_cast<uint64_t>(dt);
                std::cout << "[VideoRtp] via MRUDP from=" << dwTID
                          << " pps=" << pps
                          << " kbps=" << kbps
                          << std::endl;
                s_fwd_pkts = 0;
                s_fwd_bytes = 0;
                s_last_ms = now_ms;
            }
            long int rt = static_cast<long int>(recvtime);
            long int ft = static_cast<long int>(fb_send_time);
            return g_video_rtp_recv_callback(dwTID, pBuffer, dwLength, rt, ft);
        }
     /**
		 * 由于本模块通过pBuffer的首字节来辨析数据，所以不需要区分dwTID
		 */
        gMRUDPManager->DoRecvDataOrMessageForMRUDP(dwTID, pBuffer, dwLength);
        if(Isprint){
            //printf("current func MRUDP recv data\n");
            Isprint=false;
        }
        return true;
    }

    //获得文件上下线信息，以更新端到端数据可靠传输实体
    void ON_TERM_ON_OFF_FROM_SPEEDCONTROL(DWORD dwTID, bool bCreate) {
        //获取本终端ID
        static const DWORD dSelfTermId = GetIntegerKeyIni("Main", "DeviceID", 0);

        if (true == bCreate) {
            //终端上线
            gMRUDPManager->InitSingleEnd2EndReliableTransmission(
                    std::make_shared<MRUDPTerm2TermConfig>(dwTID, dSelfTermId));
        } else {
            //终端下线
            gMRUDPManager->RemoveSingleEnd2EndReliableTransmission(dwTID);
        }
    }

    MRUDPManager::MRUDPManager(std::string filePath) : m_filePath(filePath) {

        undeal_data_thread_flag.test_and_set();

        pthread_mutex_init(&MapMutex_, nullptr);
        pthread_mutex_init(&RecvMutex_, nullptr);
        pthread_mutex_init(&UndealDataQueueMutex_, nullptr);

        RegisterSpeedControlTermOnOffCallBack(ON_TERM_ON_OFF_FROM_SPEEDCONTROL);
        Register_SpeedControl_RecvDataCallBack(ON_RECV_DATA_FROM_NETCOMBTRANSFER);
        Register_NetCombTransferRouterCallback(ON_ROUTE_CALL_BACK_FROM_MRUDP);

        std::thread undeal_data_thread(&MRUDPManager::DoCycle, this);
        undeal_data_thread.detach();
        //win版本用了std::thread是跨平台的
        /*pthread_create();*/
    }

    MRUDPManager::~MRUDPManager() {
        //跨平台
        undeal_data_thread_flag.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        //linux
        //TODO:完善使用pthread的情况，需要对pthread进行封装，参考muduo
        pthread_mutex_destroy(&MapMutex_);
        pthread_mutex_destroy(&RecvMutex_);
        pthread_mutex_destroy(&UndealDataQueueMutex_);
    }

    bool MRUDPManager::RemoveSingleEnd2EndReliableTransmission(DWORD dwTID) {
        pthread_mutex_lock(&MapMutex_);
        auto iter = m_mapReliableUdpSocket.find(dwTID);
        if (iter != m_mapReliableUdpSocket.end()) {
            m_mapReliableUdpSocket.erase(iter);
//            printf("[TransmissionInfo]::To terminal:%d 's End2EndReliableTransmission closed\n", dwTID);
        }
        pthread_mutex_unlock(&MapMutex_);
        return true;
    }

    float MRUDPManager::ComputeAndGetRUDPLossRateByTID(DWORD remote_tid) {
        float LossRate = 0.0;
        pthread_mutex_lock(&MapMutex_);
        auto iter = m_mapReliableUdpSocket.find(remote_tid);
        if (iter != m_mapReliableUdpSocket.end()) {
            LossRate = iter->second->ComputeRetryRate();
        }
        pthread_mutex_unlock(&MapMutex_);
        return LossRate;
    }

    bool MRUDPManager::IsRUDPOnByTermId(DWORD dTermId) {
        //我们以相应的端到端实体在线作为终端在线的标准
        bool bResult = false;
        pthread_mutex_lock(&MapMutex_);
        auto iter = m_mapReliableUdpSocket.find(dTermId);
        if (iter != m_mapReliableUdpSocket.end()) {
            bResult = true;
        }
        pthread_mutex_unlock(&MapMutex_);
        return bResult;
    }

    bool MRUDPManager::SendDataBytesToTermByMRUDPWithoutReliable(DWORD dTermId, const std::shared_ptr<BYTE> &bytes,
                                                                 DWORD nLength) {
        bool bRet = false;
        DWORD SendCounts = 0;
        if (IsRUDPOnByTermId(dTermId)) {
            DWORD pLength = 0;
            std::shared_ptr<BYTE> pBytes = MRUDPEncodeBytesWithoutReliable(bytes, nLength, pLength);

            //TEST:测试视频数据传输
//            fwrite(pBytes.get(),pLength,1,fp_MRUDPEncode_send);

            while (!bRet && SendCounts < 5) {
                bRet = SendTIDDataWithSpeedControl(dTermId, pBytes, pLength);
                SendCounts++;
            }
            if (!bRet && SendCounts >= 5) {
            }
            return true;
        } else {
            return false;
        }
    }

    bool MRUDPManager::AdjustBandWidth(DWORD dTermId) {
        std::shared_ptr<End2EndReliableTransmission> pTempTrans = nullptr;
        pthread_mutex_lock(&MapMutex_);
        if (m_mapReliableUdpSocket.find(dTermId) == m_mapReliableUdpSocket.end()) {
            //说明目前还没有通往该终端的端到端可靠传输
            pthread_mutex_unlock(&MapMutex_);
            return false;
        } else {
            pTempTrans = m_mapReliableUdpSocket.at(dTermId);
        }
        pthread_mutex_unlock(&MapMutex_);
        if (pTempTrans) {
            return pTempTrans->AdjustBandWidth();
        }
        return false;
    }

    bool MRUDPManager::DealMessageWork() {
        bool bResult = false;
        std::vector<DWORD> TempRemoteTID;
        std::vector<std::shared_ptr<Force_ACK_Message>> TempACK;

        static DWORD64 DealFACKNum = 0;
        static DWORD64 UpdateTime = 0;
        static DWORD64 LastAckTime = 0;

        if ((GetTickCount() - UpdateTime) > 10000) {
            if (DealFACKNum)
//                printf("[MRUDPDebug]::%lu FACK have been deal per 10s\n", DealFACKNum);
                UpdateTime = GetTickCount();
            DealFACKNum = 0;
        }
        pthread_mutex_lock(&UndealDataQueueMutex_);
        //printf("[MRUDPDebug]::TempAck size is %lu\n", m_mapTermToAck.size());
        if (!m_mapTermToAck.empty()) {
            for (const auto &TempTermToAck: m_mapTermToAck) {
                //printf("[MRUDPDebug]::TempAckMap.first is %d,TempAck\n", TempTermToAck.first);
                TempRemoteTID.push_back(TempTermToAck.first);
                TempACK.push_back(TempTermToAck.second);
            }
            m_mapTermToAck.clear();
        }
        //printf("[Docycle线程]TempAck size After clear is %lu\n", m_mapTermToAck.size());
        pthread_mutex_unlock(&UndealDataQueueMutex_);
        std::shared_ptr<End2EndReliableTransmission> pSocketFind = nullptr;
        for (DWORD i = 0; i < TempRemoteTID.size(); i++) {
            //第一步是看是否有该TID对应的端到端可靠传输实体
            //printf("[Docycle线程]进入处理FACK的环节\n");
            pthread_mutex_lock(&MapMutex_);
            //printf("[Docycle线程]开始寻找目的终端为%d的通信实例\n", TempRemoteTID[i]);
            if (m_mapReliableUdpSocket.find(TempRemoteTID[i]) != m_mapReliableUdpSocket.end()) {
                pSocketFind = m_mapReliableUdpSocket.at(TempRemoteTID[i]);
            }
            pthread_mutex_unlock(&MapMutex_);

            if (pSocketFind) {
                //printf("[MRUDPDebug]::开始处理一个FACK,FACK的SACK区段:%zu个\n",TempACK[i]->GetSackBlocks().size());
                pSocketFind->OnReceiveMessage(TempACK[i]);
                DWORD ACKInterval = GetTickCount() - LastAckTime;
                if (ACKInterval > 100) {
//                    printf("[MRUDPDebug]::Long Time For New FACK :%d ms\n", ACKInterval);
                }
                ++DealFACKNum;
                LastAckTime = GetTickCount();
                bResult = true;
            } else {
                //printf("[Docycle线程]没有找到对应的通信实例\n");
            }
        }
        //printf("[Docycle线程]DealMessageWork with return :%d\n===============\n", bResult);
        return bResult;
    }

    bool MRUDPManager::DealDataWork() {
        /*std::shared_ptr<MrudpUndealData> TempData = nullptr;*/
        bool bResult = false;
        //printf("[MRUDPDebug]::Begin dealdataWork, m_undealData size is %zu\n", m_undealData.size());
        if (!m_undealData.empty()) {
            //TempData = nullptr;
            pthread_mutex_lock(&UndealDataQueueMutex_);
            if (!m_undealData.empty()) {
                std::shared_ptr<MrudpUndealData> &TempData = m_undealData.front();
                //找对应通信实例
                /*std::shared_ptr<End2EndReliableTransmission> pSocketFind = nullptr;*/
                pthread_mutex_lock(&MapMutex_);
                if (m_mapReliableUdpSocket.find(TempData->dwTID) != m_mapReliableUdpSocket.end()) {
                    std::shared_ptr<End2EndReliableTransmission> &pSocketFind = m_mapReliableUdpSocket.at(
                            TempData->dwTID);
                    pthread_mutex_unlock(&MapMutex_);
                    if (pSocketFind) {
                        bResult = true;
                        //开始调用其HandleIO方法
                        printf(">>-.-<< || MRUDP:MRUDPManager->pSocketFind->HandleIO\n");
                        pSocketFind->HandleIO(TempData->pBuffer, TempData->dwLength);
                    }
                } else {
                    pthread_mutex_unlock(&MapMutex_);
                }
                m_undealData.pop_front();
            }
            pthread_mutex_unlock(&UndealDataQueueMutex_);
        }
        return bResult;
    }


    //没有返回false的情况
    bool
    MRUDPManager::InitSingleEnd2EndReliableTransmission(std::shared_ptr<MRUDPTerm2TermConfig> m_role_with_channel) {
        printf("InitSingleEnd2EndReliableTransmission----------------------------\n");
        //从配置文件获得本终端的终端号
        DWORD dSelfTermId = GetIntegerKeyIni("Main", "DeviceID", 0);
        //MARK为了实现本地测试，暂时注释
        if (dSelfTermId != m_role_with_channel->GetSelfTermId()) {
            //与本端无关的端到端可靠就忽略
            return true;
        }

        //第二步是判断是否已经有通往该终端的End2EndReliableTransmission
        DWORD dRemoteTermId = m_role_with_channel->GetRemoteTermId();
        pthread_mutex_lock(&MapMutex_);
        if (m_mapReliableUdpSocket.find(dRemoteTermId) == m_mapReliableUdpSocket.end()) {
            //说明目前还没有通往该终端的RUDP通道
            std::string sFilePath = m_filePath + std::to_string(dRemoteTermId);
            std::shared_ptr<End2EndReliableTransmission> rudpSocket =
                    std::make_shared<End2EndReliableTransmission>(dSelfTermId, dRemoteTermId, sFilePath);

            //将远端的终端号与End2EndReliableTransmission的映射放入MAP
            m_mapReliableUdpSocket.insert(
                    std::pair<DWORD, std::shared_ptr<End2EndReliableTransmission>>(dRemoteTermId, rudpSocket));
            printf("[TransmissionInfo]::Init an End2EndReliableTransmission,SelfID:%d--->RemoteID:%d\n", dSelfTermId,
                   dRemoteTermId);
        }
        pthread_mutex_unlock(&MapMutex_);
        return true;
    }

    bool MRUDPManager::SendDataBytesToEndByMRUDP(DWORD dTermId, const std::shared_ptr<BYTE> &pData, DWORD nLength) {
        std::shared_ptr<End2EndReliableTransmission> pTemp = nullptr;
        pthread_mutex_lock(&MapMutex_);
        if (m_mapReliableUdpSocket.find(dTermId) == m_mapReliableUdpSocket.end()) {
            //说明目前还没有通往该终端的端到端可靠传输
            pthread_mutex_unlock(&MapMutex_);
            //debug
            printf("<o>MRUDPManager::SendDataBytesToEndByMRUDP::not find usable udp socket\n");
            return false;

        } else {
            pTemp = m_mapReliableUdpSocket.at(dTermId);
        }
        pthread_mutex_unlock(&MapMutex_);
        if (pTemp) {
            return pTemp->SendUserData(pData, nLength);
        }
        printf("<o>MRUDPManager::SendDataBytesToEndByMRUDP::pTemp is false\n");
        return false;
    }

    bool MRUDPManager::DealTempDataQueueForAllEnd2EndTransmission() {
        std::queue<std::shared_ptr<End2EndReliableTransmission>> end2end_queue;
        pthread_mutex_lock(&MapMutex_);
        for (const auto &TempTrans: m_mapReliableUdpSocket) {
            if (TempTrans.second) {
                end2end_queue.push(TempTrans.second);
            }
        }
        pthread_mutex_unlock(&MapMutex_);
        while (!end2end_queue.empty()) {
            end2end_queue.front()->UpdateRecvQueue();
            end2end_queue.pop();
        }
        return true;
    }

    bool MRUDPManager::UploadTempDataQueueForAllEnd2EndTransmission() {
        std::queue<std::shared_ptr<End2EndReliableTransmission>> end2end_queue;
        pthread_mutex_lock(&MapMutex_);
        for (const auto &TempTrans: m_mapReliableUdpSocket) {
            if (TempTrans.second) {
                end2end_queue.push(TempTrans.second);
            }
        }
        pthread_mutex_unlock(&MapMutex_);
        while (!end2end_queue.empty()) {
            end2end_queue.front()->UploadRecvQueue();
            end2end_queue.pop();
        }
        return true;
    }

    bool MRUDPManager::SendFACKForAllEnd2EndReliableTransmission() {
        //临时的队列，存储相关socket，以减少同步开销
        std::queue<std::shared_ptr<End2EndReliableTransmission>> end2end_queue;
        pthread_mutex_lock(&MapMutex_);
        for (const auto &TempTrans: m_mapReliableUdpSocket) {
            if (TempTrans.second) {
                end2end_queue.push(TempTrans.second);
            }
        }
        pthread_mutex_unlock(&MapMutex_);
        while (!end2end_queue.empty()) {
            static DWORD dwNumAck = 0;
            dwNumAck++;
            if (dwNumAck % 100 == 0) {
            }
            end2end_queue.front()->SendFACK();
            end2end_queue.pop();
        }
        return true;
    }

    //230226,可以尝试优化,或许会有较大改善.
    bool MRUDPManager::DoRecvDataOrMessageForMRUDP(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dwLength) {
        pthread_mutex_lock(&RecvMutex_);
        static DWORD clear_num = 0;
        static DWORD newData_num = 0;
        static int64_t TotalDataPacketNumInTransmission = 0;
        //std::shared_ptr<MrudpUndealData> TempData = nullptr;
        bool bResult = false;
        pthread_mutex_lock(&UndealDataQueueMutex_);
        if (IsAckMessage(pBuffer, dwLength)) {
            //printf("调用DoRecvDataOrMessageForMRUDP,Recv的是ACK包\n");
            static DWORD dwNumAck = 0;
            dwNumAck++;
            if (dwNumAck % 100 == 0) {
            }
            std::shared_ptr<Force_ACK_Message> ack_message = std::make_shared<Force_ACK_Message>();
            if (ack_message->Decode(pBuffer, dwLength))
                m_mapTermToAck[dwTID] = ack_message;
            //printf("[MRUDPDebug]::MRUDP_RecvACK from %d,SACK:%zu\n", dwTID, ack_message->GetSackBlocks().size());

        } else if (IsForceSpeedMessage(pBuffer, dwLength)) { //调整预期发送速度的指令(首字节为11)

            std::shared_ptr<ReliableForceSpeedMessage> force_speed_message = std::make_shared<ReliableForceSpeedMessage>();
            force_speed_message->Decode(pBuffer, dwLength);
            TermSpeedInfo term_speed_info;
            term_speed_info.m_remote_tid = force_speed_message->GetRemoteTid();
            term_speed_info.m_d_speed = force_speed_message->GetForceSpeed();
            bool changeResult = SpeedControlQoSSpeedInfo(term_speed_info);

        } else if (IsForceAdjustBandWidthMessage(pBuffer, dwLength)) { //手动重传指令(首字节为12)

            std::shared_ptr<ReliableForceAdjustBandWidthMessage> force_adjust_message = std::make_shared<ReliableForceAdjustBandWidthMessage>();
            force_adjust_message->Decode(pBuffer, dwLength);
            DWORD dRemoteTID = force_adjust_message->GetRemoteTid();
            AdjustBandWidth(dRemoteTID);

        } 
        else if(IsUnreliableData(pBuffer,dwLength)){
            //TEST:测试视频传输数据
//            fwrite(pBuffer.get(),dwLength,1,fp_MRUDPEncode_recv);

//            std::cout<<"[MRUDPManager] DoRecvDataOrMessageForMRUDP: Enter IsUnreliableData:"<<std::endl;
            std::shared_ptr<BYTE> pBytes = MRUDPDecodeDataWithoutReliable(pBuffer,dwLength);
        }
        else {
            printf("-=-=-==-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n"\
                   "调用DoRecvDataOrMessageForMRUDP,"\
                   "Recv的是数据包,m_undealDataLength is %zu\n",m_undealData.size());
            printf("-=-=-==-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n"\
                   "dwLength:%d, pBuffer.get()[0]:%d, pBuffer.get()[1]:%d"\
                   "\n",dwLength, pBuffer.get()[0], pBuffer.get()[1]);
            //for(int i=0;i<m_undealData.size();i++){
            //    printf("%lld ",&m_undealData[i]);
            //}
            //printf("/n");
            //for(int i=0;i<m_undealData.size();i++){
            //    printf(("%x "),&m_undealData[i]);
            //}
            //printf("\n");
            ++mStatus.TotalRecvTransmissionDataPackNum;
            if (dwLength >= 2) {
                //如果收到紧急重传的数据包，清空之前的队列，把该数据包存入队列（此时在队首）进行处理
                if (pBuffer.get()[0] == MRUDP_DATA_FIRST_BYTE_FLAG && pBuffer.get()[1] == 1) {
                    //紧急重传包直接放在队首
                    ++mStatus.RecvUrgencyReTransmissionPackNum;
                    m_undealData.push_front(
                            std::make_shared<MRUDP::MrudpUndealData>(dwTID, pBuffer, dwLength));  //在队列末尾插入新数据
                }
            }
            if (m_undealData.size() >= m_undealDataLength) //如果队列已满,删除旧数据,存入新数据
            {
                bResult = true;
                //抛弃掉队列里前四分之一的元素
                DWORD packet_need_remove = m_undealDataLength / 4;
                while (packet_need_remove != 0 && !m_undealData.empty()) {
                    m_undealData.pop_front();
                    packet_need_remove--;
                }
                //printf("\nMRUDP RecvDataList is full\n\n");
                m_undealData.push_back(
                        std::make_shared<MRUDP::MrudpUndealData>(dwTID, pBuffer, dwLength));  //在队列末尾插入新数据
            } else {
                bResult = true;
                printf("\nm_undealData.push_back(...)\n\n");
                m_undealData.push_back(std::make_shared<MRUDP::MrudpUndealData>(dwTID, pBuffer, dwLength));
            }
        }
        pthread_mutex_unlock(&UndealDataQueueMutex_);
        pthread_mutex_unlock(&RecvMutex_);
        return bResult;
    }

}
