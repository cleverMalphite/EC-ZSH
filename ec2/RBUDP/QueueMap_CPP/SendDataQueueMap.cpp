//
// Created by Kong on 2023/6/9.
//

#include "QueueMap_H/SendDataQueueMap.h"
#include "../SpeedControl/SpeedControlApi.h"
#include "map"
#include "memory"

using namespace std;

namespace RBUDP {

    bool SendDataQueueMap::AddItem(shared_ptr<RBUDPDataSenderStore> p_item, DWORD filesize) {

        p_item->AddToQueueSuccess(m_map_index - 1, filesize);
        return true;
    }

    bool SendDataQueueMap::AddItemInStorage(shared_ptr<RBUDPDataSenderStore> p_item, DWORD filesize) {
        /*m_map_size++;

        p_item->AddToQueueSuccess(m_map_index, filesize);
        p_item->SetLastRetransferTime(GetTickCount());
        m_storage->StorageData(p_item);
        p_item->SetDataNull();*/

        m_map_size++;
        pthread_mutex_lock(&m_data_tmp_queue_cs);
        m_queue_map.insert(pair<DWORD, shared_ptr<RBUDPDataSenderStore>>(m_map_index, p_item));
        pthread_mutex_unlock(&m_data_tmp_queue_cs);
        p_item->AddToQueueSuccess(m_map_index, filesize);
        m_storage->StorageData(p_item);
        p_item->SetDataNull();
        p_item->SetLastRetransferTime(GetTickCount());

        //不考虑数据过多的情况
        m_map_index++;

        return true;
    }

    bool SendDataQueueMap::SetMaxIndex(DWORD index) {
        if (index != 0) {
            m_tail_index += index;
            return true;
        }
        return false;
    }

    bool SendDataQueueMap::AddItemtoTimeout(DWORD dIndex) {
        shared_ptr<RBUDPDataSenderStore> p_item = nullptr;
        pthread_mutex_lock(&m_data_tmp_queue_cs);
        if (m_queue_map.count(dIndex) != 0) {
            p_item = m_queue_map.find(dIndex)->second;
            if (p_item != nullptr) {
                //cout << dIndex << " ";
                p_item->abletoTimeout();
                pthread_mutex_unlock(&m_data_tmp_queue_cs);
                return true;
            }
        }
        pthread_mutex_unlock(&m_data_tmp_queue_cs);
        return true;
    }

    int sum = 0;


    DWORD SendDataQueueMap::OnRecvMessageFromRecvTerm(shared_ptr<NAK_Message> nak_message) {
        DWORD d_recv_number = 0;
        d_recv_number = DeleteIndexInMap(nak_message);
        timeout_control = true;
        return d_recv_number;
    }

    DWORD st = GetTickCount(), e = 0;
    int n = 0;

    DWORD SendDataQueueMap::SendDataInSacks() {
        //GlobalCritialSection cs(m_cs);
        //m_cs;
        //DWORD d_current_time = GetTickCount();
        DWORD d_end_time = 0;
        DWORD d_resend_number = 0;

        //DWORD time_threshold = m_d_timestamp_diff_threshold;
        //if (timeout_queue.size() != 0) {
        //
        //	d_end_time = GetTickCount();
        //	if (d_end_time - d_current_time != 0) {
        //		cout << "retransmit speed:[" << 1369 * d_resend_number * 8000.0 / (d_end_time - d_current_time) / 1024 / 1024 << " Mbps]" << endl;
        //	}
        //}
        bool IsUrgent = true;
        if (timeout_queue.size() != 0) {
            for (auto iter = timeout_queue.begin(); iter != timeout_queue.end();) {
                bool res = false;
                int ret_fail_count = 0;
                pthread_mutex_lock(&m_data_tmp_queue_cs);
                if (m_queue_map.count(*iter) == 1) {
                    shared_ptr<RBUDPDataSenderStore> p_item = m_queue_map.find(*iter)->second;
                    if (p_item != nullptr) {
                        DWORD dLength = 0;
                        shared_ptr<BYTE> pTempBytes = m_storage->GetDataByIndex(*iter, dLength);
                        if (pTempBytes && pTempBytes.get() && dLength > RBUDPdata::m_nPacketLengthExceptData) {
                            pTempBytes.get()[RBUDPdata::m_nPacketUrgentFlag] = 1;
                        }
                        while (!res && ret_fail_count < 5) {
                            res = SendTIDDataWithSpeedControl(m_d_remote_term_id, pTempBytes, dLength, IsUrgent, false,true);
                            ret_fail_count++;
                            //SPDLOG_LOGGER_ERROR(mrudp_logger, "MRUDP retry count7,{}", ++temp_count_7);
                        }
                        //cout<< *iter <<"retransmit data has sent"<<endl;
                        //SPDLOG_LOGGER_WARN(rbudp_rm_logger, "{} retransmit data has send", *iter);
                        //p_item->SetLastRetransferTime(d_current_time);
                        if (res) {
                            //SPDLOG_LOGGER_WARN(rbudp_nak_logger, "resend {}", *iter);
                            iter = timeout_queue.erase(iter);
                            d_resend_number++;
                            n++;
                            e = GetTickCount();
                            if (e - st >= 1000) {
                                //SPDLOG_LOGGER_WARN(rbudp_rs_logger, "{}", n * dLength * 8.0 / 1000 / (e - st));
                                st = GetTickCount();
                                n = 0;
                            }
                        }
                    }
                } else {
                    iter++;
                }
                pthread_mutex_unlock(&m_data_tmp_queue_cs);
            }
        }
        //SPDLOG_LOGGER_WARN(rbudp_rm_logger, "resend do");

        return d_resend_number;
    }


    int recv_times = 0;

    set <DWORD> SendDataQueueMap::GetIndexInSacks(shared_ptr<NAK_Message> nak_message) {
        //TODOTODO 根据得到的NAK解析到达的数据
        set<DWORD> nak_initial;
        BYTE Flag = nak_message->GetFlag();
        DWORD Offset = nak_message->GetOffset();
        vector<DWORD> nak_blocks = nak_message->GetSackBlocks();
        vector<DWORD>::iterator iter1 = nak_blocks.begin();
        DWORD current = 0;
        int i = 0;
        if (Flag == 0) {                    //第一种解析方式
            bool blocks_in = false;

            while (i < nak_blocks.size()) {
                if (*iter1 & (1UL << 31)) {
                    i++;
                    blocks_in = true;            //此时进入区块
                    current = *iter1;
                    current &= ~(1 << 31);
                    if (current & 0x80000000) {
                        current &= 0x7fffffff;
                        //SPDLOG_LOGGER_WARN(rbudp_rm_logger, "error,{}",current);
                    }
                    //nak_blocks3.insert(current);
                    //current = 0;
                    iter1++;
                }
                if (blocks_in) {
                    while (current < *iter1) {
                        //nak_initial.insert(current + Offset);
                        nak_initial.insert(current);
                        //SPDLOG_LOGGER_WARN(rbudp_rm_logger, "recv nak:[{}]", current);
                        current++;
                    }
                    //i++;
                    blocks_in = false;
                } else {
                    i++;
                    //nak_initial.insert(*iter1+Offset);
                    nak_initial.insert(*iter1);
                    //SPDLOG_LOGGER_WARN(rbudp_rm_logger, "recv nak:({})", *iter1);
                    iter1++;
                }
            }
            //SPDLOG_LOGGER_WARN(rbudp_rs_logger, "recv nak {} end, size:{},offset:{}", recv_times++, nak_blocks.size(),Offset);



        } else {                                    //位图解析方式
            DWORD bit = 1;
            while (i < nak_blocks.size()) {

                if (bit == 0) {
                    bit = 1;
                    iter1++;
                    i++;
                    //current = 0;
                }

                if (i == nak_blocks.size()) {
                    break;
                }
                if (*iter1 & bit) {
                    //cout << lable << " ";
                    nak_initial.insert(current + Offset);
                    //fout << lable1 << " ";
                    current++;
                    //current |= bit;
                    bit *= 2;
                } else {
                    while (!(*iter1 & bit) && bit != 0) {
                        current++;
                        bit *= 2;
                        //current |= bit;
                    }

                }
                //i++;
            }
        }
        return nak_initial;
    }

    int SendDataQueueMap::DeleteIndexInMap(shared_ptr<NAK_Message> nak_message) {
        BYTE Flag = nak_message->GetFlag();
        DWORD Offset = nak_message->GetOffset();
        vector<DWORD> nak_blocks = nak_message->GetSackBlocks();
        vector<DWORD>::iterator iter1 = nak_blocks.begin();
        DWORD head = 0, tail = 0;
        DWORD d_recv_number = 0;
        DWORD last_size = 0;
        int i = 0;
        if (Flag == 0) {

            //SPDLOG_LOGGER_WARN(rbudp_rm_logger, "recv a nak");

            while (i < nak_blocks.size()) {
                if (*iter1 & (1UL << 31)) {
                    i++;
                    head = *iter1;
                    head &= ~(1 << 31);
                    iter1++;
                    tail = *iter1;
                    pthread_mutex_lock(&m_data_tmp_queue_cs);
                    m_queue_map.erase(m_queue_map.find(head), m_queue_map.find(tail));
                    m_queue_map.erase(tail);
                    pthread_mutex_unlock(&m_data_tmp_queue_cs);
                    *iter1++;
                    d_recv_number = d_recv_number + (tail - head + 1);
                    //SPDLOG_LOGGER_WARN(rbudp_rm_logger,"recv nak:{}-{}", head, tail);
                    //cout << "recv nak" << head << "-" << tail << endl;
                } else {
                    pthread_mutex_lock(&m_data_tmp_queue_cs);
                    m_queue_map.erase(*iter1);
                    pthread_mutex_unlock(&m_data_tmp_queue_cs);
                    //SPDLOG_LOGGER_WARN(rbudp_rm_logger, "recv nak:{}", *iter1);
                    //cout << "recv nak:" << *iter1 << endl;
                    last_size = *iter1;
                    iter1++;
                    d_recv_number += 1;

                }
                i++;
            }
            iter1--;

            if (last_size > rest_retransfer) {
                rest_retransfer = last_size;
            }
            //cout << "{" << m_map_size << "," << m_queue_map.size() << "}";
        }
        return d_recv_number;
    }

    bool SendDataQueueMap::DealTimeOut() {
        //GlobalCritialSection cs(m_cs);
        //m_cs;
        DWORD interval = GetTickCount() - last_timeout_time;
        if (timeout_control) {
            last_timeout_time = GetTickCount();
            pthread_mutex_lock(&m_data_tmp_queue_cs);
            int times = 0;
            auto iter = m_queue_map.begin();
            while (iter != m_queue_map.end() && iter->first <= rest_retransfer) {
                shared_ptr<RBUDPDataSenderStore> p_item = iter->second;
                //cout << iter->first << " -> " << iter->second->GetRetransferTime() << endl;
                //cout <<"{"<< p_item->GetPacketIndex() << ","<<p_item->GetDataLength() <<"} ";
                if (p_item) {
                    //先检查是否可以已经从SpeedControl中发送出去
                    if (p_item->GetabletoTimeout()) {
                        if (!p_item->AddRetransferTime()) {
                            if (times < 5000) {
                                //此时触发超时重传,将数据放入队列
                                timeout_queue.push_back(iter->first);
                                //cout << "{" << iter->first << "}";
                                times++;
                            }
                        }
                    }
                }
                iter++;
            }
            //cout << "<" << timeout_queue.size() << ">";
            pthread_mutex_unlock(&m_data_tmp_queue_cs);
            timeout_control = false;
            return true;
        } else if (interval > 500) {
            last_timeout_time = GetTickCount();
            pthread_mutex_lock(&m_data_tmp_queue_cs);
            map<DWORD, shared_ptr<RBUDP::RBUDPDataSenderStore>>::iterator iter = m_queue_map.begin();
            while (iter != m_queue_map.end()) {
                shared_ptr<RBUDPDataSenderStore> p_item = iter->second;
                //cout << iter->first << " -> " << iter->second->GetRetransferTime() << endl;
                //cout <<"{"<< p_item->GetPacketIndex() << ","<<p_item->GetDataLength() <<"} ";
                if (p_item) {
                    if (p_item->GetabletoTimeout()) {
                        if (!p_item->AddRetransferTime()) {
                            //p_item->GetRetransferTime();
                            //此时触发超时重传,将数据放入队列
                            timeout_queue.push_back(iter->first);
                            //cout << "{" << iter->first << "}";
                        }
                    }
                }
                iter++;
            }
            //cout << "<" << timeout_queue.size() << ">";
            pthread_mutex_unlock(&m_data_tmp_queue_cs);
            timeout_control = false;
            return true;
        }
        return false;
    }

    bool SendDataQueueMap::IsEmpty() {
        if (m_map_size != 0) {
            return true;
        }
        return false;
    }
}
