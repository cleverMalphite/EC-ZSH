//
// Created by Kong on 2023/6/9.
//

#include "QueueMap_H/RecvDataQueueMap.h"
#include "../SpeedControl/SpeedControlApi.h"
#include "memory"
#include "RBUDPApi.h"

using namespace std;

#define MAXSegment 4294967295

namespace RBUDP {


    void RecvDataQueueMap::DealSingleReceiveData(std::shared_ptr<RBUDPdata> data) {
        AddIndex(data);
    }

    bool RecvDataQueueMap::OnRecvDataFromLowerLayers(std::shared_ptr<RBUDPdata> p_data) {
        AddIndex(p_data);
        return false;
    }

    bool RecvDataQueueMap::OnRecvIndexFromLowerLayers(std::shared_ptr<RBUDPdata> p_data) {
        pthread_mutex_lock(&m_data_tmp_queue_cs);
        //AddIndex(p_data);
        pthread_mutex_unlock(&m_data_tmp_queue_cs);
        //SPDLOG_LOGGER_WARN(rbudp_rm_logger, "recv {} packet", p_data->GetPacketIndex());
        return false;
    }


    int send_times = 0;


    std::shared_ptr<NAK_Message> RecvDataQueueMap::TraverseNAK_WithoutSeg(DWORD dTermId) {

        std::shared_ptr<NAK_Message> m_nak = nullptr;        //存储将要发送的NAK
        {
            pthread_mutex_lock(&m_data_tmp_queue_cs);
            if (m_queue_map.empty()) {
                pthread_mutex_unlock(&m_data_tmp_queue_cs);
                return nullptr;
            }
            //将从头连续的所有数据提交到上层
            //printf("lock Traverse,m_queue size:%d\n",m_queue_size);
            set<DWORD>::iterator iter;
            if (m_queue_map.count(m_head_index) != 0) {
                iter = m_queue_map.find(m_head_index);
            } else {
                iter = m_queue_map.begin();
            }
            //DWORD current_index = 0;
            while (*iter == m_head_index) {
                DWORD dLength = 0;
                std::shared_ptr<RBUDPdata> pTempDecode = m_queue_data_tmp.find(*iter)->second;
                //std::shared_ptr<RBUDPdata> pTempDecode = make_shared<RBUDPdata>();
                //pTempDecode->Decode(pTempBytes, dLength);
                data_callback_upper.push_back(pTempDecode);
                m_queue_data_tmp.erase(*iter);
                //data_callback_upper_length.push(dLength);
                m_head_index++;
                iter++;
                if(iter == m_queue_map.end()){
                    break;
                }
            }

            if (m_queue_map.count(m_head_index) == 0) {
                //SPDLOG_LOGGER_WARN(rbudp_rm_logger, "update fail,the id we need is {} which is not exist", m_head_index);
            }

            bool function = ComputeLoss();
            //DWORD start = ComputeSeg();			//用于存放第一个数据
            DWORD start = *m_send_queue.begin();
            //Test_ycy
            function = true;
            if (function) {                    //采用区块生成方式

                int NAK_MAX_SIZE = 800;
                int nak_size = 0;
                DWORD head, tail = 0;

                vector<DWORD> nak_blocks;    //生成nak区块
                if (m_send_queue.size() > 0) {
                    iter = m_send_queue.begin();
                    bool blocks_in = false;    //为true时表明正在遍历一个NAK区块
                    DWORD current = 0;
                    DWORD next_index = 0;
                    while (next(iter) != m_send_queue.end()) {
                        //DWORD dLength = 0;
                        //if (ComputeGap(*iter, start)) {
                        //	m_tmp_queue.insert(start-*iter);
                        //	iter++;
                        //	break;
                        //}
                        current = *iter;
                        next_index = *next(iter);
                        if (current + 1 != next_index) {        //此时不连续
                            nak_blocks.push_back(current);
                            //SPDLOG_LOGGER_WARN(rbudp_nak_logger, "send nak:({})", current);
                            //cout << "send nak:" << current << endl;
                            nak_size++;
                            blocks_in = false;
                        } else if (!blocks_in) {        //此时连续
                            //current -= start;
                            current |= 0x80000000;        //将第一位置为1
                            nak_blocks.push_back(current);
                            //SPDLOG_LOGGER_WARN(rbudp_nak_logger, "send nak:[{}]", (current&0x7fffffff));
                            //cout << "send nak:" << (current & 0x7fffffff) << endl;
                            nak_size++;
                            blocks_in = true;
                        } else if (blocks_in) {
                            //SPDLOG_LOGGER_WARN(rbudp_nak_logger, "send nak:[{}]", *iter);
                            //cout << "send nak:" << *iter << endl;
                        }
                        /*if (next(iter) != m_send_queue.end())*/
                        iter++;
                        //else {
                        //	nak_blocks.push_back(*iter - start);
                        //	SPDLOG_LOGGER_WARN(rbudp_rm_logger, "send nak:({})", *iter);
                        //	nak_size++;
                        //	break;
                        //}
                        if (nak_size >= NAK_MAX_SIZE) {
                            if (!blocks_in) {
                                tail = *iter;
                                break;
                            }
                        }
                    }
                    nak_blocks.push_back(*iter);
                    //SPDLOG_LOGGER_WARN(rbudp_nak_logger, "send nak:<{}>", *iter);
                    //cout << "send nak:" << *iter << endl;
                }
                if (nak_blocks.size() != 0) {
                    //SPDLOG_LOGGER_WARN(rbudp_logger, "send nak {} end,size:{},offset：{}", send_times++, nak_blocks.size(), start);
                    //cout<<"send nak "<<send_times++<<" end,size: "<<nak_blocks.size()<<endl;
                }
                m_nak = make_shared<NAK_Message>(0, 0, nak_blocks);
                if (tail != 0) {
                    m_send_queue.erase(m_send_queue.begin(), m_send_queue.find(tail));
                    m_send_queue.erase(*iter);
                } else {
                    m_send_queue.clear();
                }
            } else {
                bool flag_can = false;
                vector<DWORD> nak_blocks2;    //生成nak区块
                int lable = 0;
                if (m_send_queue.size() >= 2400) {
                    iter = m_send_queue.begin();
                    DWORD bit = 1;
                    DWORD current = 0;
                    DWORD size = 0;

                    while (iter != m_send_queue.end() && size < 2400) {
                        if (bit == 0) {
                            bit = 1;
                            nak_blocks2.push_back(current);
                            current = 0;
                        }

                        if (lable == *iter) {
                            current |= bit;
                            lable++;
                            iter++;
                            size++;
                            bit *= 2;
                            flag_can = true;
                        } else {
                            while ((lable != *iter) && bit < 2147483647) {
                                lable++;
                                size++;
                                bit *= 2;
                            }
                            flag_can = false;
                        }
                    }
                    if (lable == *prev(iter)) {
                        current |= bit;
                    }
                    nak_blocks2.push_back(current);
                }
                m_nak = make_shared<NAK_Message>(1, start, nak_blocks2);
                if (m_send_queue.size() == 2400) {
                    m_send_queue.clear();
                } else if (m_send_queue.size() > 2400) {
                    if (!flag_can) {
                        iter++;
                    }
                    m_send_queue.erase(m_send_queue.begin(), iter);
                }
            }
            //shared_ptr<RBUDPdata> p_data_temp = nullptr;
            //退出临界区后将数据返回给上层模块



            if (next(m_queue_map.find(m_head_index - 1)) != m_queue_map.end()) {        //此时可删除的最后一个元素不在队尾,也就是说他下一个还有元素
                m_queue_map.erase(m_queue_map.find(m_tail_index), m_queue_map.find(m_head_index - 1));
                m_queue_map.erase(m_head_index - 1);
            } else {
                m_queue_map.erase(m_queue_map.find(m_tail_index), m_queue_map.end());
            }
            if (m_tail_index != m_head_index) {
                //	//SPDLOG_LOGGER_WARN(rbudp_logger, "{} packets is acked and updated,current packet:{}", m_head_index - m_tail_index, m_head_index);
                //cout << "{" << m_queue_size << "," << m_tail_index << "," << m_head_index << "}"<<endl;
            }

            pthread_mutex_unlock(&m_data_tmp_queue_cs);
            //printf("unlock Traverse\n");
            m_tail_index = m_head_index;
        }
        return m_nak;
    }

    bool RecvDataQueueMap::AddItem(std::shared_ptr<RBUDPdata> p_item) {
        m_storage->StorageData(p_item);
        p_item->SetDataNull();

        return false;
    }

    bool RecvDataQueueMap::AddIndex(std::shared_ptr<RBUDPdata> p_index) {

        const DWORD dIndex = p_index->GetPacketIndex();
        pthread_mutex_lock(&m_data_tmp_queue_cs);
        if (m_queue_map.count(dIndex) == 0) {


            //printf("lock Addindex\n");
            m_queue_map.insert(dIndex);
            m_send_queue.insert(dIndex);
            m_queue_data_tmp.insert(std::pair<DWORD, std::shared_ptr<RBUDPdata>>(dIndex, p_index));

            //printf("unlock Addindex\n");
            m_queue_size++;
        }
        pthread_mutex_unlock(&m_data_tmp_queue_cs);
        return false;
    }

    DWORD RecvDataQueueMap::ComputeAvg() {
        if (m_send_queue.empty()) {
            return 0;
        }
        DWORD avg = 0;
        std::set<DWORD>::iterator iter = m_send_queue.begin();
        int i = 0;
        while (i < m_send_queue.size()) {
            avg += *iter;
            iter++;
            i++;
        }
        avg /= m_send_queue.size();
        return avg;
    }

    DWORD RecvDataQueueMap::ComputeSeg() {
        DWORD avg = ComputeAvg();
        set<DWORD>::iterator iter = m_send_queue.begin();
        while (ComputeGap(*iter, avg)) {
            iter++;
        }
        return *iter;
    }

    bool RecvDataQueueMap::ComputeLoss() {
        if (m_send_queue.size() >= m_recv_count) {
            return true;
        }
        return false;
    }

    bool RecvDataQueueMap::ComputeGap(DWORD iter, DWORD avg) {
        if (iter >= avg) {
            return false;
        } else {
            if (avg - iter >= 1000000)
                return true;
            return false;
        }
    }

    void RecvDataQueueMap::UploadDataPacket(DWORD dTermId) {
        pthread_mutex_lock(&m_data_tmp_queue_cs);
        //printf("lock Update\n");
        //printf("[MRUDPDebug]::StartUploadData\n");
        auto uploadqueue = data_callback_upper;
        data_callback_upper.clear();
        pthread_mutex_unlock(&m_data_tmp_queue_cs);
        //printf("unlock Update\n");
        while (!uploadqueue.empty()) {
            //统计数据包数量
            std::shared_ptr<RBUDPdata> &p_data_temp = uploadqueue.front();

            printf("_________________________\n");
            printf("+++++++++++++++++++++++++\n");
            printf("^^^^^^^^^^^^^^^^^^^^^^^^^\n");
            printf("RBUDP:recv reliable data ...\n");
            printf("Check Data type:\n");
            //检查数据类型，如果不是数据服务类型数据，则交给BigDataTransfer的回调
            //if(rbudp_dataserver_check(p_data_temp->GetData(), p_data_temp->GetDataLength())==false){
                printf("CallBackDataFunc => BigDataTransfer\n");
                CallBackDataFunc(dTermId, p_data_temp->GetData(),
                                    p_data_temp->GetDataLength());
                printf("...CallBackDataFunc...ok\n");
            //}
            //else { //如果是数据服务类型数据，则交给DATA SERVER的回调
            //    //MRUDP_DATASERVER_CALLBACK
            //    //数据回调服务对应的回调调用 240910-houlc spin_server_callback
            //    printf("spin_server_callback => DATA SERVER\n");
            //    bool spin_res = rbudp_spin_server_callback(dTermId,
            //                        p_data_temp->GetData(),
            //                        p_data_temp->GetDataLength()) ;
            //    printf("...spin_server_callback...ok(res:%d)\n", spin_res);
            //}
            printf("#########################\n");

            uploadqueue.pop_front();
        }
    }

}
