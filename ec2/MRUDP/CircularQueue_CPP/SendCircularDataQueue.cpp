//
// Created by 王炳棋 on 2022/12/24.
//

#include <utility>

#include "MRUDPApi.h"
#include "../SpeedControl/SpeedControlApi.h"
#include "CircularQueue_H/SendCircularDataQueue.h"

namespace MRUDP {
    /*DWORD ChangeTimeThreshold(DWORD time_thershold_temp) {
        return time_thershold_temp;
    }*/

    void SendCircularDataQueue::AddAndComputeRequestReTransferFrequency(DWORD num) {
        if (0 == m_send_num_request) {
            m_send_num_request_timestamp_start = GetTickCount();
        }
        m_send_num_request += num;
        if (m_send_num_request >= STATISTIC_TIME) {
            DWORD time_used = GetTickCount() - m_send_num_request_timestamp_start;
            m_send_num_request_frequency = (m_send_num_request * 1.0 / time_used) * 1000;
            m_send_num_request = 0;
            m_send_num_request_timestamp_start = GetTickCount();
        }
    }

    void SendCircularDataQueue::AddAndComputeTimeoutReTransferFrequency(DWORD num) {
        if (0 == m_send_num_timeout) {
            m_send_num_timeout_timestamp_start = GetTickCount();
        }
        m_send_num_timeout += num;
        if (m_send_num_timeout >= STATISTIC_TIME) {
            DWORD time_used = GetTickCount() - m_send_num_timeout_timestamp_start;
            m_send_num_timeout_frequency = (m_send_num_timeout * 1.0 / time_used) * 1000;
            m_send_num_timeout = 0;
            m_send_num_timeout_timestamp_start = GetTickCount();
        }
    }

    DWORD m_start_time1 = 0;
    DWORD m_end_time1 = 0;
    bool IsStart1 = false;

    bool SendCircularDataQueue::AddItem(const std::shared_ptr<ReliableUdpDataSenderStore> &pItem) {
        pthread_mutex_lock(&Mutex_);
        //先判断队列是否已满或流量窗口已满
        //printf("[MRUDPDebug]::SendQueue size/capacity is %d/%d\n", m_queue_size, m_queue_capacity);
        if (m_queue_size > m_queue_windows_expect_num || m_queue_size == m_queue_capacity) {
            pthread_mutex_unlock(&Mutex_);
            //printf("[MRUDPDebug]::发送队列满\n");
            return false;
        }

        m_queue_size++;
        m_queue_array[m_queue_tail] = pItem;

        pItem->AddToQueueSuccess(m_queue_tail, m_dCycleIndex);
        if (m_queue_tail == m_queue_capacity - 1) {
            m_dCycleIndex++;
        }

        //printf("[MRUDPDebug]::Index %d,Cycle:%d Packet Is ADD to SendQue\n", m_queue_tail, m_dCycleIndex);
        //pItem经过encode后被加入到了存储介质中
        m_storage->StorageData(pItem);
        //然后把pItem里关联到数据的智能指针赋值为nullptr，这一步可以节省内存
        pItem->SetDataNull();
        pItem->SetLastReTransferTime(GetTickCount());
        m_queue_tail = (m_queue_tail + 1) % m_queue_capacity;
        pthread_mutex_unlock(&Mutex_);
        return true;
    }

    std::shared_ptr<ReliableUdpDataSenderStore> SendCircularDataQueue::GetItemByIndex(DWORD dIndex) {
        if (dIndex > 0 && dIndex <= m_queue_capacity - 1) {
            return m_queue_array.at(dIndex);
        }
        return nullptr;
    }

    bool SendCircularDataQueue::SetItemByIndex(DWORD dIndex, std::shared_ptr<ReliableUdpDataSenderStore> pItem) {
        if (/*dIndex >= 0* &&*/ dIndex <= m_queue_capacity - 1) {
            m_queue_array[dIndex] = std::move(pItem);
            return true;
        }
        return false;
    }

    bool SendCircularDataQueue::SetHeadIndex(const std::shared_ptr<const Force_ACK_Message> &pFackMessage) {
        if (0 == m_queue_size) {
            return true;
        }
        /**
		 * 首先要确保其确认的尾包在本数据接收队列的有效区间内，分两步，一步看尾包时间戳，尾包时间戳如果不对那就肯定不对
		 */
        //const DWORD d_tail_packet_timestamp = pFackMessage->GetTailPacketTimeStamp();
        const DWORD d_tail_cycle_index = pFackMessage->GetTailCycleIndex();
        const DWORD d_tail_packet_index = pFackMessage->GetTailIndexNumber();
        //printf("[MRUDPDebug]::TailPacketCycleIndex is %d, TailPacketIndex is %d\n", d_tail_cycle_index, d_tail_packet_index);
        std::shared_ptr<ReliableUdpDataSenderStore> pItem = m_queue_array[d_tail_packet_index];
        //printf("[MRUDPDebug]::pItem.CycleIndex is %d\n", pItem->GetCycleIndex());
        if (pItem->GetCycleIndex() != d_tail_cycle_index || !IsIndexInValid(d_tail_packet_index)) {
            //如果1. 已经收到数据接收队列相应包; 2.或者FACK所携带的传输轮次与相应包传输轮次不等；
            // 3.尾包序号不在数据接收队列有效部分序列范围。         =>那么直接返回
            //printf("[MRUDPDebug]::SetHeadIndex failed,Queue_head is %d,Queue_tail is %d\n", m_queue_head, m_queue_tail);
            return false;
        } else {
            DWORD old_queue_size = m_queue_size;    //之前的队列长度
            m_queue_head = (d_tail_packet_index + 1) % m_queue_capacity;
            const CircularDataQueueIndexStatus internal_index_status = GetStatusValidQueue();
            if (Queue_Head_Index_Greater_Than_Queue_Tail_Index == internal_index_status) {
                m_queue_size = m_queue_tail + m_queue_capacity - m_queue_head;
            } else if (Queue_Head_Index_Less_Than_Queue_Tail_Index == internal_index_status) {
                m_queue_size = m_queue_tail - m_queue_head;
            } else {
                m_queue_size = 0;
            }
            //更新流量窗口
            m_queue_windows_expect_num += (old_queue_size - m_queue_size);
            m_queue_windows_expect_num =
                    m_queue_windows_expect_num > m_queue_capacity ? m_queue_capacity : m_queue_windows_expect_num;
            //printf("[MRUDPDebug]::New m_queue_windows_expect_num is %d\n", m_queue_windows_expect_num);
        }
        return true;
    }

    bool SendCircularDataQueue::NewSetHeadIndex(const std::shared_ptr<const Force_ACK_Message> &pFackMessage) {
        if (0 == m_queue_size) {
            return true;
        }
        /**
		 * 首先要确保其确认的尾包在本数据接收队列的有效区间内，分两步，一步看尾包时间戳，尾包时间戳如果不对那就肯定不对
		 */
        //const DWORD d_tail_packet_timestamp = pFackMessage->GetTailPacketTimeStamp();
        const DWORD d_tail_cycle_index = pFackMessage->GetTailCycleIndex() + 1;
        const DWORD d_tail_packet_index =
                (pFackMessage->GetTailIndexNumber() - 1 + m_queue_capacity) % m_queue_capacity;
        //printf("[MRUDPDebug]::TailPacketCycleIndex is %d, TailPacketIndex is %d\n", d_tail_cycle_index, d_tail_packet_index);
        std::shared_ptr<ReliableUdpDataSenderStore> pItem = m_queue_array[d_tail_packet_index];
        //printf("[MRUDPDebug]::pItem.CycleIndex is %d\n", pItem->GetCycleIndex());
        if (pItem->GetCycleIndex() != d_tail_cycle_index || !IsIndexInValid(d_tail_packet_index)) {
            //如果1. 已经收到数据接收队列相应包; 2.或者FACK所携带的传输轮次与相应包传输轮次不等；
            // 3.尾包序号不在数据接收队列有效部分序列范围。         =>那么直接返回
            //printf("[MRUDPDebug]::SetHeadIndex failed,Queue_head is %d,Queue_tail is %d\n", m_queue_head, m_queue_tail);
            return false;
        } else {
            DWORD old_queue_size = m_queue_size;    //之前的队列长度
            m_queue_head = (d_tail_packet_index + 1) % m_queue_capacity;
            const CircularDataQueueIndexStatus internal_index_status = GetStatusValidQueue();
            if (Queue_Head_Index_Greater_Than_Queue_Tail_Index == internal_index_status) {
                m_queue_size = m_queue_tail + m_queue_capacity - m_queue_head;
            } else if (Queue_Head_Index_Less_Than_Queue_Tail_Index == internal_index_status) {
                m_queue_size = m_queue_tail - m_queue_head;
            } else {
                m_queue_size = 0;
            }
            //更新流量窗口
            m_queue_windows_expect_num += (old_queue_size - m_queue_size);
            m_queue_windows_expect_num =
                    m_queue_windows_expect_num > m_queue_capacity ? m_queue_capacity : m_queue_windows_expect_num;
            //printf("[MRUDPDebug]::New m_queue_windows_expect_num is %d\n", m_queue_windows_expect_num);
        }
        return true;
    }

    bool SendCircularDataQueue::SetHeadIndex(DWORD dIndex) {

        if (dIndex == m_queue_head) {
            return true;
        }
        if (dIndex == m_queue_tail) {
            m_queue_head = m_queue_tail;
            m_queue_size = 0;
            return true;
        }

        CircularDataQueueIndexStatus index_stauts = GetStatusValidQueue();
        switch (index_stauts) {
            case Queue_Head_Index_Greater_Than_Queue_Tail_Index: {
                if (dIndex > m_queue_tail && dIndex < m_queue_head) {
                    return false;
                } //超出有效范围
                m_queue_head = dIndex;
                if (m_queue_head < m_queue_tail) {
                    m_queue_size = m_queue_tail - m_queue_head;
                } else if (m_queue_head > m_queue_tail) {
                    m_queue_size = m_queue_tail + (m_queue_capacity - m_queue_head);
                } //千万要注意要先减去，避免溢出
                else {
                    m_queue_size = 0;
                }
            }
            case Queue_Head_Index_Less_Than_Queue_Tail_Index: {
                if (dIndex < m_queue_head || dIndex > m_queue_tail) {
                    return false;
                }
                m_queue_head = dIndex;
                m_queue_size = m_queue_tail - m_queue_head;
            }
                return true;
            case Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Full: {
                m_queue_head = dIndex;
                const CircularDataQueueIndexStatus internal_index_status = GetStatusValidQueue();
                if (Queue_Head_Index_Greater_Than_Queue_Tail_Index == internal_index_status) {
                    m_queue_size = m_queue_tail + m_queue_capacity - m_queue_head;
                } else if (Queue_Head_Index_Less_Than_Queue_Tail_Index == internal_index_status) {
                    m_queue_size = m_queue_tail - m_queue_head;
                } else {
                    m_queue_size = 0;
                }
            }
                return true;
            case Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Zero: {
            }
                return true;
            case Wrong_Head_Tail_Index_Status: {
            }
            default:
                return false;
        }
    }

    DWORD SendCircularDataQueue::OnRecvMessageFromRecvTerm(const std::shared_ptr<Force_ACK_Message> &FackMessage) {
        pthread_mutex_lock(&Mutex_);
        bool ret = false;
        DWORD ret_fail_count = 0;
        DWORD d_resend_number = 0;
        //获取累计确认包序号tail_index
        /*printf("[MRUDPDebug]::收到一个FACK,d_tail_index = %d,SACK块长度:%zu,本地发送队列长度:%d\n", d_tail_index,
               FackMessage->GetSackBlocks().size(), m_queue_size);*/
        bool isWindowChannge = SetHeadIndex(FackMessage);
        if (isWindowChannge) {
            m_last_confirm_time = GetTickCount();
        }
        bool isUrgentTransfer = false;
        static bool isUrgentTransferStatic = false;
        if (isWindowChannge) {
            if (m_queue_size != 0) {
                /*SPDLOG_LOGGER_WARN(SelfAdaptionLogger, "Packets to term {}  [{} - {}](totally {} packets) are unconfirmed!",
                                   m_d_remote_term_id, m_queue_head, m_queue_tail, m_queue_size);*/
                /*printf("[MRUDPDebug]::Packets to term %d:[%d - %d](totally %d packets) are unconfirmed!", m_d_remote_term_id, m_queue_head,
                       m_queue_tail,
                       m_queue_size);*/
            }
            m_recv_ack_invalid_number++;
            //收到一定数量的ack，尾包没有发生变化，将窗口减小并进行紧急重传
            if (RECV_ACK_INVALID_MAX_COUNT_NUM == m_recv_ack_invalid_number) {
                m_recv_ack_invalid_number = 0;
                //流量窗口减半
                /*m_queue_windows_expect_num = m_queue_windows_expect_num * WINDOWS_EXPECT_NUM_MULTI_WHEN_LOSS_DETECTED;
                if (WINDOWS_EXPECT_NUM_MIN > m_queue_windows_expect_num)
                {
                    m_queue_windows_expect_num = WINDOWS_EXPECT_NUM_MIN;
                }*/
                DWORD logger_old_window_num = m_queue_windows_expect_num;
                m_queue_windows_expect_num = WINDOWS_EXPECT_NUM_MIN;
                /*SPDLOG_LOGGER_WARN(MRUDPLogger, "Down send window from {} to {}!", logger_old_window_num,
                                   m_queue_windows_expect_num);
                SPDLOG_LOGGER_WARN(MRUDPLogger, "Last Data in MRUDP SendList has not change for ten ack!");*/
                isUrgentTransferStatic = true;
            }
        } else {
            isUrgentTransferStatic = false;
        }
        if (isUrgentTransferStatic) {
            isUrgentTransfer = true;
        }
        /*if (isWindowChange && 0 != d_tail_packet_timestamp)
        {
            shared_ptr<ReliableUdpDataSenderStore> p_item = m_queue_array[d_tail_index];
            //如果阈值更新，那么动态更新超时阈值
            DWORD32 dTailConfirmTime = GetTickCount() - d_tail_packet_timestamp;//减去ACK生成间隙

            DWORD32 p_transfer_single_rtt_times = 1.5 * p_item->GetRetransferTimeThreshold() + 2 * force_ack_message->GetReceiveTimes();	//force_ack_message的receive times说明是实实在在的一个RTT（因为ACK要传过来）,其它的无效传输包暂定为一个单向传输时延（因为ACK发到了客户端）
            m_d_timestamp_diff_threshold = dTailConfirmTime / p_transfer_single_rtt_times;
            if (m_d_timestamp_diff_threshold > 20)
            {
                m_d_timestamp_diff_threshold = 20;
            }
        }
        else
        {
            if (m_d_timestamp_diff_threshold > 20)
            {
                m_d_timestamp_diff_threshold = 20;
            }
        }

        m_d_timestamp_diff_threshold = TIME_THRESHOLD;
        */
        //重发Head_Index后的n个包
        DWORD i = 0;
        DWORD d_retransfer_for_request = 0;    //记录因为请求重传而重新发送的数据包个数
        DWORD d_retransfer_for_timeout = 0;    //记录因为超时重传而重新发送的数据包个数

        //先重传sack块内的间隔，就因为请求重传的数据包数目.
        std::vector<DWORD> sack_blocks = FackMessage->GetSackBlocks();
        DWORD sack_block_size = sack_blocks.size();
        DWORD sack_first_index = m_queue_head;
        DWORD sack_second_index = 0;
        if (sack_block_size >= 2) {
            //printf("[MRUDPDebug]::SACK区块数目:%d个,根据SACK进行请求重传\n", sack_block_size);
            sack_second_index = sack_blocks[0];
            d_retransfer_for_request += SendDataInSacks(sack_first_index, sack_second_index);
            d_resend_number = d_retransfer_for_request + d_retransfer_for_timeout;
            for (DWORD j = 1; j < sack_block_size - 1; j += 2) {
                sack_first_index = sack_blocks[j];
                sack_second_index = sack_blocks[j + 1];
                d_retransfer_for_request += SendDataInSacks(sack_first_index, sack_second_index);
                d_resend_number = d_retransfer_for_request + d_retransfer_for_timeout;

                /*if (d_resend_number > m_d_scan_next_queue_head_for_retransfer)
                {
                    AddAndComputeRequestReTransferFrequency(d_retransfer_for_request);
                    return d_resend_number;
                }*/
            }
            AddAndComputeRequestReTransferFrequency(d_retransfer_for_request);
            pthread_mutex_unlock(&Mutex_);
            return d_resend_number;
        } else if (sack_block_size < 2) {
            //只有没有SACK的时候才发送尾包之后的数据包
            //printf("[MRUDPDebug]::SACK区块数目不足:%d个\n", sack_block_size);
            //再重传最后一个SACK块后由于超时而重传的数据包数目
            if (m_queue_size > 800) {//800?
                Retransfer_UnAcked_Packets_in_SendWindow(isUrgentTransfer, d_resend_number, d_retransfer_for_request,
                                                         d_retransfer_for_timeout);
            } else {
                DWORD delay = GetTickCount() - m_last_confirm_time;
                if (delay >= 400) {
                    Retransfer_UnAcked_Packets_in_SendWindow(isUrgentTransfer, d_resend_number,
                                                             d_retransfer_for_request,
                                                             d_retransfer_for_timeout);
                }
            }
            /*Retransfer_UnAcked_Packets_in_SendWindow(isUrgentTransfer, d_resend_number,
                                                     d_retransfer_for_request,
                                                     d_retransfer_for_timeout);*/
        }
        AddAndComputeRequestReTransferFrequency(d_retransfer_for_request);
        AddAndComputeTimeoutReTransferFrequency(d_retransfer_for_timeout);
        pthread_mutex_unlock(&Mutex_);
        return d_resend_number;
        //return 0;
    }


    DWORD SendCircularDataQueue::Retransfer_UnAcked_Packets_in_SendWindow
            (bool isUrgentTransfer, DWORD &d_resend_number, DWORD &d_retransfer_for_request,
             DWORD &d_retransfer_for_timeout) {
        //printf("[AutoTestDebug]::Retransfer_UnAcked_Packets_in_SendWindow\n");
        bool ret = false;
        DWORD ret_fail_count = 0;
        DWORD first_timeout_index = m_queue_head;
        DWORD d_current_time = GetTickCount();
        DWORD time_threshold = m_d_timestamp_diff_threshold;
        //printf("[MRUDPDebug]::Retransfer_UnAcked_Packets_in_SendWindow 调用,time_threshold is %d ms\n", time_threshold);
        if (m_queue_size == m_queue_capacity && m_queue_head == first_timeout_index) {
            std::shared_ptr<ReliableUdpDataSenderStore> pItem = m_queue_array[first_timeout_index];
            if ((pItem->GetLastReTransferTime() + time_threshold) < d_current_time) {
                DWORD dLength = 0;
                //d_resend_number++;
                //切记不要调用ReliableUdpData的编码方法，这是因为在上面发送数据时ReliableUdpData中的数据部分就已经被置为nullptr
                shared_ptr<BYTE> pTempBytes = m_storage->GetDataByIndex(first_timeout_index, dLength);
                if (pTempBytes && pTempBytes.get() && dLength >= ReliableUdpData::m_nPacketLengthExceptData) {
                    pTempBytes.get()[ReliableUdpData::m_nPacketLengthReason] = MRUDPTransferReason::TRANSFER_FOR_TIMEOUT;
                }
                bool isForceSleep = false;
                if (isUrgentTransfer) {
                    pTempBytes.get()[ReliableUdpData::m_nPacketUrgentFlag] = 1;
                    while (!ret && ret_fail_count < 5) {
                        ret = SendTIDDataWithSpeedControl(m_d_remote_term_id, pTempBytes, dLength, isUrgentTransfer,
                                                          isForceSleep, false);
                        ret_fail_count++;
                        //SPDLOG_LOGGER_ERROR(MRUDOLogger, "MRUDP retry count1,{},sendpackindex:{}", ret_fail_count, first_timeout_index);
                    }
                    if (!ret && ret_fail_count >= 5) {
//                        SPDLOG_LOGGER_ERROR(MRUDPLogger, "MRUDP senddata fail after five send.");
                    }
                    isUrgentTransfer = false;
                    //将ret置为false,ret_fail_count置为0,避免该重传对下一轮重传的影响
                    ret = false;
                    ret_fail_count = 0;
                } else {
                    while (!ret && ret_fail_count < 5) {
                        ret = SendTIDDataWithSpeedControl(m_d_remote_term_id, pTempBytes, dLength,
                                                          true/*isUrgentTransfer*/, isForceSleep,false);
                        ret_fail_count++;
                        //SPDLOG_LOGGER_ERROR(MRUDPLogger, "MRUDP retry count2,{},sendpackindex:{}", ret_fail_count, first_timeout_index);
                    }
                    if (!ret && ret_fail_count >= 5) {
//                        SPDLOG_LOGGER_ERROR(MRUDPLogger, "MRUDP senddata fail after five send.");
                    }
                    //将ret置为false,ret_fail_count置为0,避免该重传对下一轮重传的影响
                    ret = false;
                    ret_fail_count = 0;

                    //之前是为了调试重传是否有误加的,可删
                    DWORD dReadPacketIndex = ReadData(pTempBytes.get() + 1);
                    if (dReadPacketIndex != first_timeout_index) {
                        /*SPDLOG_LOGGER_WARN(MRUDPLogger, "packet index {}, while read index {}", first_timeout_index,
                                           dReadPacketIndex);*/
                    }
                    DWORD dCycleIndex = ReadData(pTempBytes.get() + 5);
                    pItem->ChangeReTransferTimeThreshold();
                    pItem->SetLastReTransferTime(d_current_time);
                    d_retransfer_for_timeout++;
                    d_resend_number++;
                    if (d_resend_number > m_d_scan_next_queue_head_for_retransfer) {
                        AddAndComputeRequestReTransferFrequency(d_retransfer_for_request);
                        AddAndComputeTimeoutReTransferFrequency(d_retransfer_for_timeout);
                        return d_resend_number;
                    }
                }
                //time_threshold = ChangeTimeThreshold(time_threshold);
                first_timeout_index = (first_timeout_index + 1) % m_queue_capacity;
            }
        }

        DWORD d_bianli_packets_num = 1;
        time_threshold = m_d_timestamp_diff_threshold;
        for (auto k = first_timeout_index; k != m_queue_tail; k = (k + 1) % m_queue_capacity) {
            d_bianli_packets_num++;
            if (d_bianli_packets_num >= 800 && (d_bianli_packets_num % time_threshold_change_packets_num) == 0) {
                time_threshold += time_threshold_change;
            }
            std::shared_ptr<ReliableUdpDataSenderStore> pItem = m_queue_array[k];
            if ((pItem->GetLastReTransferTime() + time_threshold) < d_current_time) {
                DWORD dLength = 0;
                //d_resend_number++;
                //切记不要调用ReliableUdpData的编码方法，这是因为在上面发送数据时ReliableUdpData中的数据部分就已经被置为nullptr
                shared_ptr<BYTE> pTempBytes = m_storage->GetDataByIndex(k, dLength);
                if (pTempBytes && pTempBytes.get() && dLength >= ReliableUdpData::m_nPacketLengthExceptData) {
                    pTempBytes.get()[ReliableUdpData::m_nPacketLengthReason] = MRUDPTransferReason::TRANSFER_FOR_TIMEOUT;
                }
                bool isForceSleep = false;
                if (m_queue_size >= m_queue_windows_expect_num) {
                    //isForceSleep = true;
                }
                if (isUrgentTransfer) {
                    pTempBytes.get()[ReliableUdpData::m_nPacketUrgentFlag] = 1;
                    //ret = SendTIDDataWithSpeedControl(m_d_remote_term_id, pTempBytes, dLength, isUrgentTransfer, isForceSleep);
                    while (!ret && ret_fail_count < 5) {
                        //printf("[MRUDPDebug]::MRUDP Retry3, SendPacketIndex:%d\n", k);
                        ret = SendTIDDataWithSpeedControl(m_d_remote_term_id, pTempBytes, dLength, isUrgentTransfer,
                                                          isForceSleep,false);
                        ret_fail_count++;
                        //SPDLOG_LOGGER_ERROR(mrudp_logger, "MRUDP retry count3,{},sendpackindex:{}", ret_fail_count,k);
                    }
                    if (!ret && ret_fail_count >= 5) {
//                        SPDLOG_LOGGER_ERROR(MRUDPLogger, "MRUDP senddata fail after five send.");
                    }
                    isUrgentTransfer = false;
                    //将ret置为false,ret_fail_count置为0,避免该重传对下一轮重传的影响
                    ret = false;
                    ret_fail_count = 0;
                } else {
                    //ret = SendTIDDataWithSpeedControl(m_d_remote_term_id, pTempBytes, dLength, isUrgentTransfer, isForceSleep);
                    while (!ret && ret_fail_count < 5) {
                        //printf("[MRUDPDebug]::MRUDP Retry4, SendPacketIndex:%d\n", k);
                        ret = SendTIDDataWithSpeedControl(m_d_remote_term_id, pTempBytes, dLength,
                                                          true/*isUrgentTransfer*/,
                                                          isForceSleep,false);
                        ret_fail_count++;
                        //SPDLOG_LOGGER_ERROR(mrudp_logger, "MRUDP retry count4,{},sendpackindex:{}", ret_fail_count, k);
                    }
                    if (!ret && ret_fail_count >= 5) {
//                        SPDLOG_LOGGER_ERROR(MRUDPLogger, "MRUDP senddata fail after five send.");
                    }
                    //将ret置为false,ret_fail_count置为0,避免该重传对下一轮重传的影响
                    ret = false;
                    ret_fail_count = 0;
                }

                DWORD dReadPacketIndex = ReadData(pTempBytes.get() + 1);
                if (dReadPacketIndex != k) {
                    //SPDLOG_LOGGER_WARN(mrudp_logger, "packet index {}, while read index {}", k, dReadPacketIndex);
                }
                DWORD dCycleIndex = ReadData(pTempBytes.get() + 5);
                pItem->ChangeReTransferTimeThreshold();
                pItem->SetLastReTransferTime(d_current_time);
                d_retransfer_for_timeout++;
                d_resend_number++;
                if (d_resend_number > m_d_scan_next_queue_head_for_retransfer) {
                    AddAndComputeRequestReTransferFrequency(d_retransfer_for_request);
                    AddAndComputeTimeoutReTransferFrequency(d_retransfer_for_timeout);
                    return d_resend_number;
                }
            }
            //time_threshold = ChangeTimeThreshold(time_threshold);
        }
        //warning: control reaches end of non-void function
        //随便先给个return值吧，虽然感觉返回值似乎没有用上
        return 0;
    }

    bool SendCircularDataQueue::SendAllUnACKPackets() {
        //qos?
        pthread_mutex_lock(&Mutex_);
        if (m_queue_size == m_queue_capacity) {
            DWORD index = m_queue_head;
            DWORD nLength = 0;
            std::shared_ptr<BYTE> pBuffer = m_storage->GetDataByIndex(index, nLength);
            SendTIDDataWithSpeedControl(m_d_remote_term_id, pBuffer, nLength);
            index = (index + 1) % m_queue_capacity;
            for (; index != m_queue_tail; index = (index + 1) % m_queue_capacity) {
                pBuffer = m_storage->GetDataByIndex(index, nLength);
                SendTIDDataWithSpeedControl(m_d_remote_term_id, pBuffer, nLength);
            }
        } else if (m_queue_size != 0) {
            DWORD index = 0;
            DWORD nLength = 0;
            std::shared_ptr<BYTE> pBuffer = nullptr;
            for (index = m_queue_head; index != m_queue_tail; index = (index + 1) % m_queue_capacity) {
                pBuffer = m_storage->GetDataByIndex(index, nLength);
                SendTIDDataWithSpeedControl(m_d_remote_term_id, pBuffer, nLength);
            }
        } else {

        }
        pthread_mutex_unlock(&Mutex_);
        return true;
    }

    DWORD temp_count_9 = 0;

    DWORD SendCircularDataQueue::SendDataInSacks(DWORD firstIndex, DWORD lastIndex) {
        DWORD d_current_time = GetTickCount();
        DWORD d_resend_number = 0;
        bool ret = false;
        DWORD ret_fail_count = 0;
        DWORD time_threshold = m_d_timestamp_diff_threshold;
        for (auto t = firstIndex; t != lastIndex; t = ((t + 1) % m_queue_capacity)) {
            shared_ptr<ReliableUdpDataSenderStore> pItem = m_queue_array[t];
            if ((pItem->GetLastReTransferTime() + time_threshold) < d_current_time) {
                DWORD dLength = 0;
                d_resend_number++;
                //切记不要调用ReliableUdpData的编码方法，这是因为在上面发送数据时ReliableUdpData中的数据部分就已经被置为nullptr
                shared_ptr<BYTE> pTempBytes = m_storage->GetDataByIndex(t, dLength);
                if (pTempBytes && pTempBytes.get() && dLength >= ReliableUdpData::m_nPacketLengthExceptData) {
                    pTempBytes.get()[ReliableUdpData::m_nPacketLengthReason] = MRUDPTransferReason::TRANSFER_FOR_REQUEST;
                }
                //SendTIDDataWithSpeedControl(m_d_remote_term_id, pTempBytes, dLength);
                while (!ret && ret_fail_count < 5) {
                    ret = SendTIDDataWithSpeedControl(m_d_remote_term_id, pTempBytes, dLength, true);
                    ret_fail_count++;
                    //SPDLOG_LOGGER_ERROR(mrudp_logger, "MRUDP retry count4,{}", ++temp_count_4);
                }
                if (!ret && ret_fail_count >= 5) {
//                    SPDLOG_LOGGER_ERROR(MRUDPLogger, "MRUDP senddata fail after five send(SACK).");
                }
                //将ret置为false,ret_fail_count置为0,避免该重传对下一轮重传的影响
                ret = false;
                ret_fail_count = 0;
                pItem->SetLastReTransferTime(d_current_time);
                pItem->ChangeReTransferTimeThreshold();
//                SPDLOG_LOGGER_ERROR(MRUDPLogger, "MRUDP SendDataInSacks count,{}", ++temp_count_9);
            }
            //time_threshold = ChangeTimeThreshold(time_threshold);
        }
        m_d_timestamp_diff_threshold = time_threshold;
        return d_resend_number;
    }

/**
 * 不包括队尾元素
 */
    bool SendCircularDataQueue::IsIndexInValid(DWORD dIndex) const {
        if (0 == m_queue_size) {
            return false;
        }
        if (m_queue_head > m_queue_tail) {
            if (dIndex >= m_queue_tail && dIndex < m_queue_head) { return false; }
            else { return true; }
        }

        if (m_queue_head < m_queue_tail) {
            if (dIndex >= m_queue_head && dIndex < m_queue_tail) { return true; }
            else { return false; }
        }

        if (m_queue_head == m_queue_tail) {
            //特殊情况，有效队首、队尾元素索引相等
            if (m_queue_capacity == m_queue_size) {
                return true;    //队列满为在有效范围内
            } else {
                return false;  //队列空则不在有效范围内
            }
        }
        return true;
    }


    CircularDataQueueIndexStatus SendCircularDataQueue::GetStatusValidQueue() const {
        if (m_queue_head > m_queue_tail) {
            return Queue_Head_Index_Greater_Than_Queue_Tail_Index;
        }

        if (m_queue_head < m_queue_tail) {
            return Queue_Head_Index_Less_Than_Queue_Tail_Index;
        }

        if (m_queue_head == m_queue_tail) {
            if (m_queue_capacity == m_queue_size) {
                return Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Full;
            } else {
                return Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Zero;
            }
        }

        return Wrong_Head_Tail_Index_Status;
    }

}
