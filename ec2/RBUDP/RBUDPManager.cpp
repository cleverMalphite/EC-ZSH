//
// Created by Kong on 2023/6/9.
//

#include "RBUDPManager.h"
#include "queue"
#include "memory"
#include <iostream>
#include "../NetCombTransfer/NetCombTransferApi.h"
#include "../SpeedControl/SpeedControlApi.h"
#include "../Util/VideoRtp_callback.h"
#include "../Util/AudioRtp_callback.h"
#include "../Util/SystemTimeFunc.h"
#include <thread>

namespace RBUDP {
    extern shared_ptr<RBUDPManager> g_pRBUDPManager;
    extern pthread_mutex_t g_cs_for_rbudp;

    void ON_ROUTE_CALL_BACK_FROM_RBUDP(DWORD dwSourceTID, DWORD dwDestinationTID, UINT m_nNop, UINT m_nQoSSend,
                                       UINT m_nQosRecv, bool bSetted) {
        //获取本终端ID
        static const DWORD dSelfTermId = GetIntegerKeyIni("Main", "DeviceID", 0);
        if (bSetted) {
            g_pRBUDPManager->InitSingleEnd2EndReliableTransmission(
                    std::make_shared<CRBUDPTerm2TermConfig>(dwDestinationTID, dSelfTermId));
        } else {
            g_pRBUDPManager->RemoveSingleEnd2EndReliableTransmission(dwDestinationTID);
        }

    }
    bool Isprint=true;
    bool
    ON_RECV_DATA_FROM_NETCOMBTRANSFER(DWORD dwTID, const shared_ptr<BYTE> &pBuffer, DWORD dwLength, long int &recvtime,
                                      long int &fb_send_time) {
        if (dwLength <= 0) {
            return true;
        }
        const BYTE *raw = pBuffer.get();
        if (raw && raw[0] == 0x00 && g_video_rtp_recv_callback &&
            ((dwLength >= 13 && ((raw[1] & 0xC0) == 0x80)) ||
             (dwLength >= 1 + sizeof(uint64_t) + 12 && ((raw[1 + sizeof(uint64_t)] & 0xC0) == 0x80)))) {
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
                std::cout << "[VideoRtp] via RBUDP from=" << dwTID
                          << " pps=" << pps
                          << " kbps=" << kbps
                          << std::endl;
                s_fwd_pkts = 0;
                s_fwd_bytes = 0;
                s_last_ms = now_ms;
            }
            return g_video_rtp_recv_callback(dwTID, pBuffer, dwLength, recvtime, fb_send_time);
        }
        if (raw && raw[0] == 0x02 && g_audio_rtp_recv_callback &&
            ((dwLength >= 13 && ((raw[1] & 0xC0) == 0x80)) ||
             (dwLength >= 1 + sizeof(uint64_t) + 12 && ((raw[1 + sizeof(uint64_t)] & 0xC0) == 0x80)))) {
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
                std::cout << "[AudioRtp] via RBUDP from=" << dwTID
                          << " pps=" << pps
                          << " kbps=" << kbps
                          << std::endl;
                s_fwd_pkts = 0;
                s_fwd_bytes = 0;
                s_last_ms = now_ms;
            }
            return g_audio_rtp_recv_callback(dwTID, pBuffer, dwLength, recvtime, fb_send_time);
        }

        g_pRBUDPManager->DoRecvDataOrMessageForRBUDP(dwTID, pBuffer, dwLength);
        if(Isprint){
            //printf("current func RBUDP recv data\n");
            Isprint=false;
        }


        return true;
    }

    void ON_TERM_ON_OFF_FROM_SPEEDCONTROL(DWORD dwTID, bool bCreate) {
        static const DWORD dSelfTermId = GetIntegerKeyIni("Main", "Device", 0);
        if (true == bCreate) {
            //终端上线
            g_pRBUDPManager->InitSingleEnd2EndReliableTransmission(
                    std::make_shared<CRBUDPTerm2TermConfig>(dwTID, dSelfTermId));
        } else {
            //终端下线
            g_pRBUDPManager->RemoveSingleEnd2EndReliableTransmission(dwTID);
        }
    }

    void ON_RECV_DATA_FROM_SPEEDCONTROL(DWORD dwTID, DWORD dIndex) {
        //g_pRBUDPManager->DoRecvdataFromSpeedControl(dwTID, dIndex);
        RBUDPIndex index(dwTID, dIndex);
        //pthread_mutex_lock(&g_pRBUDPManager->m_temp_index_queue_cs);
        //g_pRBUDPManager->tmp_sendIndex.push(index);
        g_pRBUDPManager->IndexquePush(index);
        //添加了一个临界区的操作，接下来把pop的调用的时候也要重新改一下
        //g_pRBUDPManager->sendindexLength++;
        //pthread_mutex_unlock(&g_pRBUDPManager->m_temp_index_queue_cs);

    }

    RBUDPManager::RBUDPManager(std::string szFilePath) {

        undeal_data_thread_flag.test_and_set();
        pthread_mutex_init(&m_cs_map, nullptr);
        pthread_mutex_init(&m_undeal_data_queue_cs, nullptr);
        pthread_mutex_init(&m_temp_data_queue_cs, nullptr);
        pthread_mutex_init(&m_recv_cs, nullptr);
        pthread_mutex_init(&m_temp_index_queue_cs, nullptr);
        m_filePath = szFilePath;
        //m_fileSize = szFileSize;
        Register_SpeedControl_RecvDataCallBack(ON_RECV_DATA_FROM_NETCOMBTRANSFER);
        RegisterSpeedControlTermOnOffCallBack(ON_TERM_ON_OFF_FROM_SPEEDCONTROL);
        RegisterSpeedControlRBUDPDataCallBack(ON_RECV_DATA_FROM_SPEEDCONTROL);
        Register_NetCombTransferRouterCallback(ON_ROUTE_CALL_BACK_FROM_RBUDP);

        std::thread undeal_data_thread(&RBUDPManager::DoCycle, this);
        std::thread undeal_nak_thread(&RBUDPManager::DoNAK, this);
        std::thread undeal_timeout_thread(&RBUDPManager::DoTimeout, this);
        std::thread undeal_index_thread(&RBUDPManager::DoIndex, this);
        undeal_data_thread.detach();
        undeal_nak_thread.detach();
        undeal_timeout_thread.detach();
        undeal_index_thread.detach();
    }

    RBUDPManager::~RBUDPManager() {
        undeal_data_thread_flag.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        pthread_mutex_destroy(&m_cs_map);
        pthread_mutex_destroy(&m_undeal_data_queue_cs);
        pthread_mutex_destroy(&m_recv_cs);
        pthread_mutex_destroy(&m_temp_index_queue_cs);
        pthread_mutex_destroy(&m_temp_data_queue_cs);
    }


    bool
    RBUDPManager::InitSingleEnd2EndReliableTransmission(std::shared_ptr<CRBUDPTerm2TermConfig> m_role_with_channel) {
        //先获得本终端的终端号
        DWORD dSelfTermId = GetIntegerKeyIni("Main", "DeviceID", 0);
        if (dSelfTermId != m_role_with_channel->GetSelfTermId()) {
            //与本端无关的端到端可靠就忽略
            return true;
        }

        //第二步是判断是否已经有通往该终端的End2EndReliableTransmission
        DWORD dRemoteTermId = m_role_with_channel->GetRemoteTermId();
        pthread_mutex_lock(&m_cs_map);
        if (m_mapReliableUdpSocket.find(dRemoteTermId) == m_mapReliableUdpSocket.end()) {
            //说明目前还没有通往该终端的RUDP通道
            std::string sFilePath = m_filePath + ::to_string(dRemoteTermId);
            shared_ptr<End2EndReliableTransmission> rudp_socket = make_shared<End2EndReliableTransmission>(dSelfTermId,
                                                                                                           dRemoteTermId,
                                                                                                           sFilePath);

            //将远端的终端号与End2EndReliableTransmission的映射放入MAP
            m_mapReliableUdpSocket.insert(
                    std::pair<DWORD, std::shared_ptr<End2EndReliableTransmission>>(dRemoteTermId, rudp_socket));
            cout << "Init an End2EndReliableTransmission,SelfID:" << dSelfTermId << "--->RemoteID:" << dRemoteTermId
                 << endl;
        }
        pthread_mutex_unlock(&m_cs_map);
        pthread_mutex_lock(&m_undeal_data_queue_cs);
        m_RecvIndex.clear();
        pthread_mutex_unlock(&m_undeal_data_queue_cs);
        std::queue<RBUDP::RBUDPIndex> emptyQueue;
        tmp_sendIndex.swap(emptyQueue);
        std::queue<shared_ptr<NAK_Message>> emptyQueue1;
        m_Nak_que.swap(emptyQueue1);
        return true;
    }

    bool RBUDPManager::RemoveSingleEnd2EndReliableTransmission(DWORD dwTID) {
        pthread_mutex_lock(&m_cs_map);
        auto iter = m_mapReliableUdpSocket.find(dwTID);
        if (iter != m_mapReliableUdpSocket.end()) {
            if (dwTID == 104) {
                cout << "本次传输丢包率为：" << iter->second->ComputeRetryRate() << "%" << endl;
            }
            m_mapReliableUdpSocket.erase(iter);
            cout << "To terminal:" << dwTID << " 's End2EndReliableTransmission closed" << endl;
        }

        pthread_mutex_unlock(&m_cs_map);
        pthread_mutex_lock(&m_undeal_data_queue_cs);
        m_RecvIndex.clear();
        pthread_mutex_unlock(&m_undeal_data_queue_cs);
        std::queue<RBUDP::RBUDPIndex> emptyQueue;
        tmp_sendIndex.swap(emptyQueue);
        std::queue<shared_ptr<NAK_Message>> emptyQueue1;
        m_Nak_que.swap(emptyQueue1);
        return true;
    }

    bool RBUDPManager::SendNAKForAllEnd2EndReliableTransmission() {
        //TEST_YCY

        //TEST_TCY
        std::queue<std::shared_ptr<End2EndReliableTransmission>> end2end_queue;
        std::map<DWORD, std::shared_ptr<End2EndReliableTransmission>>::iterator iter;
        pthread_mutex_lock(&m_cs_map);
        for (iter = m_mapReliableUdpSocket.begin(); iter != m_mapReliableUdpSocket.end(); iter++) {
            if (iter->second) {
                end2end_queue.push(iter->second);
            }
        }
        pthread_mutex_unlock(&m_cs_map);

        while (end2end_queue.size() > 0) {
            end2end_queue.front()->SendNAK();
            end2end_queue.pop();
        }
        return true;
    }

    bool RBUDPManager::DoRecvDataOrMessageForRBUDP(DWORD dwTID, std::shared_ptr<BYTE> pBuffer, DWORD dwLength) {
        //GlobalCritialSection m_cs(m_recv_cs);
        std::shared_ptr<RBUDPUndealData> tmpData = nullptr;
        bool bResult = false;
        pthread_mutex_lock(&m_undeal_data_queue_cs);
        if (IsAckMessage(pBuffer, dwLength)) {
            std::shared_ptr<NAK_Message> nak_message = make_shared<NAK_Message>();
            if (nak_message->Decode(pBuffer, dwLength)) {
                m_mapTermNak[dwTID] = nak_message;
                m_Nak_que.push(nak_message);
            }//TODO
        } else {
            shared_ptr<RBUDPdata> pdata = make_shared<RBUDPdata>();
            pdata->Decode(pBuffer, dwLength);
            current_index = pdata->GetPacketIndex();
            //SPDLOG_LOGGER_WARN(rbudp_nak_logger, "recv packet {},RecvIndex times {}", current_index, m_RecvIndex.count(current_index));

            //之前没有这个数据包号
            if (dwLength >= 2 && m_RecvIndex.count(current_index) == 0) {
                /*if (current_index <= 10) {
                    cout << "{" << current_index << "," << m_RecvIndex.size() << "}";
                }*/
                if (current_index != 0) {
                    m_RecvIndex.insert(current_index);
                    //cout << "【RBUDPManager::DoRecvData】recv-1: " << pdata->GetPacketIndex() << endl;
                }

                if (pBuffer.get()[0] == MRUDP_DATA_FIRST_BYTE_FLAG && pBuffer.get()[1] == 1) {
                    m_undealData.push_front(make_shared<RBUDP::RBUDPUndealData>(dwTID, pBuffer, dwLength));
                    //SPDLOG_LOGGER_WARN(rbudp_logger, "reRecv packet");
                } else
                    m_undealData.push_back(make_shared<RBUDP::RBUDPUndealData>(dwTID, pBuffer, dwLength));

                //SPDLOG_LOGGER_WARN(rbudp_logger, "recv packet {}", pdata->GetPacketIndex());
            } else {
                //SPDLOG_LOGGER_WARN(rbudp_logger, "rerecv {}", current_index);
                //cout << "【RBUDPManager::DoRecvData】recv-2: " << pdata->GetPacketIndex() << endl;
            }
        }
        pthread_mutex_unlock(&m_undeal_data_queue_cs);
        return false;
    }

    bool RBUDPManager::SendDataBytesToEndByRBUDP(DWORD dTermId, const std::shared_ptr<BYTE> &pData, DWORD nLength
                                                 ) {
        std::shared_ptr<End2EndReliableTransmission> pTemp = nullptr;
        pthread_mutex_lock(&m_cs_map);
        if (m_mapReliableUdpSocket.find(dTermId) != m_mapReliableUdpSocket.end()) {
            pTemp = m_mapReliableUdpSocket.at(dTermId);
        } else {
            pthread_mutex_unlock(&m_cs_map);
            printf("<o>RBUDPManager::SendDataBytesToEndByRBUDP::not find usable udp socket\n");
            return false;
        }
        pthread_mutex_unlock(&m_cs_map);
        if (pTemp) {
            //cout << "begin to send" << endl;
            return pTemp->SendUserData(pData, nLength);
        }
        printf("<o>RBUDPManager::SendDataBytesToEndByRBUDP::pTemp is false\n");
        return false;
    }

    bool RBUDPManager::DoRecvdataFromSpeedControl() {
        bool res = false;
        //DWORD size = sendindexLength;

        while (!IndexIsEmpty()) {
            RBUDPIndex index = IndexqueFront();
            std::shared_ptr<End2EndReliableTransmission> pTemp = nullptr;
            pthread_mutex_lock(&m_cs_map);
            if (m_mapReliableUdpSocket.find(index.TID) != m_mapReliableUdpSocket.end()) {
                pTemp = m_mapReliableUdpSocket.at(index.TID);
            } else {
                pthread_mutex_unlock(&m_cs_map);
                return false;
            }
            pthread_mutex_unlock(&m_cs_map);
            if (pTemp) {
                res = pTemp->OnReceiveIndex(index.Index);
                if (res) {
                    IndexquePop();
                    //SPDLOG_LOGGER_WARN(rbudp_nak_logger, "send {}", index.Index);
                }
                //return res;
            }
        }
        return true;
    }


    bool RBUDPManager::IsRUDPOnByTermId(DWORD term_id) {
        //我们以相应的端到端实体在线作为终端在线的标准
        bool result = false;
        pthread_mutex_lock(&m_cs_map);
        auto iter = m_mapReliableUdpSocket.find(term_id);
        if (iter != m_mapReliableUdpSocket.end()) {
            result = true;
        }
        pthread_mutex_unlock(&m_cs_map);
        return result;
    }

    bool
    RBUDPManager::SendDataBytesToTermByRBUDPWithoutReliable(DWORD dTermId, std::shared_ptr<BYTE> bytes, DWORD nLength) {
        bool ret = false;
        DWORD ret_fail_count = 0;
        if (IsRUDPOnByTermId(dTermId)) {
            //终端在线，加上第一字节标志位和字节数，直接发出去
            DWORD p_length = 0;
            shared_ptr<BYTE> pbytes = RBUDP_Encode_Bytes_Without_Reliable(bytes, nLength, p_length);
            while (!ret && ret_fail_count < 5) {
                ret = SendTIDDataWithSpeedControl(dTermId, pbytes, p_length, false, false, true);
                ret_fail_count++;
            }
            if (!ret && ret_fail_count >= 5) {
                //SPDLOG_LOGGER_WARN(mrudp_logger, "MRUDP send data WithoutReliable fail.");
            }

            return true;
        } else {
            return false;
        }
    }

    int push_in = 0;

    bool RBUDPManager::DealDataWork() {
        std::shared_ptr<RBUDPUndealData> tmpData = nullptr;
        bool bResult = false;
        pthread_mutex_lock(&m_undeal_data_queue_cs);

        if (m_undealData.empty()) {
            tmpData = nullptr;
        } else {
            tmpData = m_undealData.front();
            m_undealData.pop_front();
            //cout << "【RBUDPManager::DealDataWork】 undealdata size:" << m_undealData.size() << endl;
        }
        pthread_mutex_unlock(&m_undeal_data_queue_cs);
        if (nullptr != tmpData) {
            //第一步是看是否有该TID对应的端到端可靠传输实体
            shared_ptr<End2EndReliableTransmission> pSocketFind = nullptr;
            pthread_mutex_lock(&m_cs_map);
            if (m_mapReliableUdpSocket.find(tmpData->dwTID) != m_mapReliableUdpSocket.end()) {
                pSocketFind = m_mapReliableUdpSocket.at(tmpData->dwTID);
            }
            pthread_mutex_unlock(&m_cs_map);

            if (pSocketFind) {
                bResult = true;
                //开始调用其HandleIO方法
                pSocketFind->HandleIO(tmpData->pBuffer, tmpData->dwLength);
            }
        }
        //SPDLOG_LOGGER_WARN(rbudp_rs_logger, "tmp size:{}",m_undealData.size());
        return bResult;
    }

    //bool RBUDPManager::DealIndexWork() {
    //	std::shared_ptr<RBUDPUndealData> tmpData = nullptr;
    //	bool bResult = false;
    //	pthread_mutex_lock(&m_undeal_data_queue_cs);

    //	if (m_undealData.empty())
    //	{
    //		tmpData = nullptr;
    //	}
    //	else
    //	{
    //		tmpData = m_undealData.front();
    //		m_undealData.pop_front();
    //		m_dealindexData.push(tmpData);
    //	}
    //	pthread_mutex_unlock(&m_undeal_data_queue_cs);
    //	if (nullptr != tmpData)
    //	{
    //		//第一步是看是否有该TID对应的端到端可靠传输实体
    //		shared_ptr<End2EndReliableTransmission> pSocketFind = nullptr;
    //		pthread_mutex_lock(&m_cs_map);
    //		if (m_mapReliableUdpSocket.find(tmpData->dwTID) != m_mapReliableUdpSocket.end())
    //		{
    //			pSocketFind = m_mapReliableUdpSocket.at(tmpData->dwTID);
    //		}
    //		pthread_mutex_unlock(&m_cs_map);

    //		if (pSocketFind)
    //		{
    //			bResult = true;
    //			//开始调用其HandleIO方法
    //			//pSocketFind->OnReceiveIndex(tmpData->pBuffer, tmpData->dwLength);
    //		}
    //	}
    //	return bResult;
    //}

    bool RBUDPManager::DealMessageWork() {
        bool bResult = false;
        std::vector<DWORD> tmpRemoteTID;
        std::vector<std::shared_ptr<NAK_Message>> tmpNak;
        pthread_mutex_lock(&m_undeal_data_queue_cs);
        //将所有数据全部取出，将对端终端号和NAK分别存储值相应位置
        auto iter = m_mapTermNak.begin();
        for (iter; iter != m_mapTermNak.end(); iter++) {
            tmpRemoteTID.push_back(iter->first);
            tmpNak.push_back(iter->second);
        }
        m_mapTermNak.clear();
        pthread_mutex_unlock(&m_undeal_data_queue_cs);
        shared_ptr<End2EndReliableTransmission> pSocketFind = nullptr;
        for (DWORD i = 0; i < tmpRemoteTID.size(); i++) {
            pthread_mutex_lock(&m_cs_map);
            if (m_mapReliableUdpSocket.find(tmpRemoteTID[i]) != m_mapReliableUdpSocket.end()) {
                pSocketFind = m_mapReliableUdpSocket.at(tmpRemoteTID[i]);
            }
            pthread_mutex_unlock(&m_cs_map);

            if (pSocketFind) {
                //SPDLOG_LOGGER_WARN(rbudp_logger, "the nak from {} is recved", tmpRemoteTID[i]);
                //m_Nak_que.push(tmpNak[i]);
                //pSocketFind->OnReceiveMessage(tmpNak[i]);
                pSocket_hasFind = pSocketFind;
                bResult = true;
            }
        }
        return bResult;
    }

    bool RBUDPManager::DealTimeOut() {
        shared_ptr<End2EndReliableTransmission> pSocketFind = nullptr;
        map<DWORD, shared_ptr<End2EndReliableTransmission>>::iterator iter;
        pthread_mutex_lock(&m_cs_map);
        //应该改成先放到另一个vector中减少占用
        for (iter = m_mapReliableUdpSocket.begin(); iter != m_mapReliableUdpSocket.end(); iter++) {
            pSocketFind = iter->second;
            pSocketFind->OnCheckTimeOut();
        }
        pthread_mutex_unlock(&m_cs_map);
        return false;
    }

    void RBUDPManager::DoCycle() {
        const int MAX_DEAL_DATA_ONE_TIME = 10;
        bool NeedSleep = true;
        int i;
        while (undeal_data_thread_flag.test_and_set()) {
            DealMessageWork() ? NeedSleep = false : NeedSleep = true;
            i = 0;
            while (MAX_DEAL_DATA_ONE_TIME > i && DealDataWork()) {
                NeedSleep = false;
                i++;
            }
            if (NeedSleep) {
                usleep(50);
            }
        }
    }

    void RBUDPManager::DoIndex() {
        bool NeedSleep = true;

        while (undeal_data_thread_flag.test_and_set()) {
            DoRecvdataFromSpeedControl() ? NeedSleep = false : NeedSleep = true;
            if (NeedSleep) {
                usleep(50);
            }
        }
    }

    void RBUDPManager::DoNAK() {
        bool NeedSleep = true;
        while (undeal_data_thread_flag.test_and_set()) {
            DealNak() ? NeedSleep = false : NeedSleep = true;
            if (NeedSleep) {
                usleep(50);
            }
        }
    }

    bool RBUDPManager::DealNak() {
        pthread_mutex_lock(&m_undeal_data_queue_cs);
        if (!m_Nak_que.empty() && pSocket_hasFind != nullptr) {
            std::shared_ptr<NAK_Message> tmp_nak = m_Nak_que.front();
            pSocket_hasFind->OnReceiveMessage(tmp_nak);
            m_Nak_que.pop();
            pthread_mutex_unlock(&m_undeal_data_queue_cs);
            return true;
        } else {
            pthread_mutex_unlock(&m_undeal_data_queue_cs);
            return false;
        }
    }

    void RBUDPManager::DoTimeout() {
        //std::this_thread::sleep_for(std::chrono::seconds(5));
        while (undeal_data_thread_flag.test_and_set()) {
            //DoRecvdataFromSpeedControl();
            DealTimeOut();
            //std::this_thread::sleep_for(std::chrono::microseconds(50));
            usleep(50000);
        }
    }

    bool RBUDPManager::UploadTempDataQueueForAllEnd2EndTransmission() {
        std::queue<std::shared_ptr<End2EndReliableTransmission>> end2end_queue;
        pthread_mutex_lock(&m_cs_map);
        for (const auto &TempTrans: m_mapReliableUdpSocket) {
            if (TempTrans.second) {
                end2end_queue.push(TempTrans.second);
            }
        }
        pthread_mutex_unlock(&m_cs_map);
        while (!end2end_queue.empty()) {
            end2end_queue.front()->UploadRecvcQueue();
            end2end_queue.pop();
        }
        return true;
    }

}
