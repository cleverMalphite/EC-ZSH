//
// Created by 王炳棋 on 2022/12/25.
//
#include <queue>
#include <utility>
#include "MRUDPApi.h"
#include "../SpeedControl/SpeedControlApi.h"
#include "CircularQueue_H/RecvCircularDataQueue.h"
#include "MRUDPManager.h"


namespace MRUDP {
    extern std::shared_ptr<MRUDPManager> gMRUDPManager;

    //TODO：改进？
    void RecvCircularDataQueue::DealReceiveData() {
        //先对临时数据接收队列进行排序 TODO
        //假设接收过程中数据多是按照序列号升序到达的，且我们的目的就是先处理序列号大的数据，
        //所以这里每次取得都是vector尾部的元素，然后将其删除，同时基于上述假设，结合效率度量，这里不再手动排序
        pthread_mutex_lock(&TempDataMutex_);
        DWORD queue_size = m_queue_data_temp.size();
        printf("[MRUDPDebug]::临时数据接收队列长度为%d\n", queue_size);
        pthread_mutex_unlock(&TempDataMutex_);
        /*std::shared_ptr<ReliableUdpDataReceiverStore> pReliableData = nullptr;*/
        while (0 < queue_size) {
            pthread_mutex_lock(&TempDataMutex_);
            /*const std::shared_ptr<ReliableUdpDataReceiverStore>& pReliableData = m_queue_data_temp.front();
            DealSingleReceiveData(pReliableData);*/
            printf("\n\nDealSingleReceiveData\n");
            DealSingleReceiveData(m_queue_data_temp.front());
            m_queue_data_temp.pop();
            queue_size = m_queue_data_temp.size();
            pthread_mutex_unlock(&TempDataMutex_);
        }
    }

    void RecvCircularDataQueue::NewDealReceiveData() {
        pthread_mutex_lock(&TempDataMutex_);
        //printf("[MRUDPDebug]::NewDealReceiveData, "\
        //       "TempDataSize is %zu,m_recv_queue tail is %d/%d\n",
        //       m_queue_data_temp.size(),m_queue_tail,m_queue_capacity);
        //Only 1000 packets will be processed per cycle at most
        const static DWORD OnceDealMaxNum = 5000;
        static DWORD DealCounts = 0;
        while (!m_queue_data_temp.empty() && DealCounts < OnceDealMaxNum) {
            NewDealSingleReceiveData(m_queue_data_temp.front());
            m_queue_data_temp.pop();
            ++DealCounts;
        }
        DealCounts = 0;
        pthread_mutex_unlock(&TempDataMutex_);
    }

    void RecvCircularDataQueue::DealSingleReceiveData(const std::shared_ptr<ReliableUdpDataReceiverStore> &pData) {
        const DWORD dIndex = pData->GetPacketIndex();
        printf("[MRUDPDebug]::CirculaerQ: DealSingleReceiveData\n");
        //MARK:???
        /*if (pData->GetPacketUrgentFlag() == 1) { //如果是紧急重传包，比较该包是否是当前需要的包
            std::shared_ptr<ReliableUdpDataReceiverStore>& pTemp = m_queue_array[dIndex];
        }*/

        const std::shared_ptr<ReliableUdpDataReceiverStore> &pTemp = m_queue_array[dIndex];    //取出目前队列内的该元素
        const DWORD &pDataCycleIndex = pData->GetCycleIndex();
        const DWORD &pTempCycleIndex = pTemp->GetCycleIndex();
        if ((/*pData->GetCycleIndex()*/pDataCycleIndex > /*pTemp->GetCycleIndex()*/pTempCycleIndex) &&
            !pTemp->IsAcked()) {
            //新来的包时效性较强，那么就把该包替换掉
            m_queue_array[dIndex] = pData;    //注意这里千万不能把元素里面的数据部分置为nullptr
            BYTE transfer_reason = pData->GetTransferReason();
            switch (transfer_reason) {
                case MRUDPTransferReason::TRANSFER_FOR_INIT:
                    m_queue_array[dIndex]->SetSacked();
                    break;
                case MRUDPTransferReason::TRANSFER_FOR_REQUEST:
                    m_queue_array[dIndex]->AddRequestRetransferTime();
                    m_queue_array[dIndex]->SetSacked();
                    //AddAndComputeRequestReTransferFrequency();
                    break;
                case MRUDPTransferReason::TRANSFER_FOR_TIMEOUT:
                    m_queue_array[dIndex]->AddTimeOutReTransferTime();
                    m_queue_array[dIndex]->SetSacked();
                    //AddAndComputeTimeoutReTransferFrequency();
                    break;
                default:
                    m_queue_array[dIndex]->AddTimeOutReTransferTime();
                    m_queue_array[dIndex]->SetSacked();
                    //AddAndComputeTimeoutReTransferFrequency();
                    break;
            }
            //更改循环队列内容，如果dIndex在原有效队列外，还要把新加入队列的值设置为false，否则设置为true
            if (IsIndexInValid(dIndex)) {
                //如果在有效队列内，不需要做什么
                //printf("[MRUDPDebug]::收到了在有效队列内的序号为:%d的数据包,队列长度现在是:%d\n", dIndex, m_queue_size);
                AddAndComputeValidTransferFrequency();
            } else {
                //如果不在有效队列内，那么需要扩展队列，并从原tail（包括tail）起，把新增加入队列的元素（此元素除外）的收到标志置为false
                while (m_queue_tail != dIndex) {
                    SetElementNotReceiveByIndex(m_queue_tail);    //TODO 这里需要清空一些元素里的成员变量
                    m_queue_tail = (m_queue_tail + 1) % m_queue_capacity;
                    m_queue_size++;
                }
                m_queue_tail = (dIndex + 1) % m_queue_capacity;
                m_queue_size++;
                //重新标注索引
                /*printf("[MRUDPDebug]::收到了不在有效队列内的序号为:%d的数据包,拓展队列,队首为%d,队尾为%d,队列长度现在是:%d\n", dIndex,
                       m_queue_head, m_queue_tail, m_queue_size);*/
            }
        } else if (/*pData->GetCycleIndex()*/ pDataCycleIndex == /*pTemp->GetCycleIndex()*/pTempCycleIndex) {
            //说明此数据包已经被接收，然而还没有被累积确认
            BYTE transfer_reason = pData->GetTransferReason();
            switch (transfer_reason) {
                case MRUDPTransferReason::TRANSFER_FOR_INIT:
                    printf("预期之外的数据\n");
                    break;
                case MRUDPTransferReason::TRANSFER_FOR_REQUEST:
                    m_queue_array[dIndex]->AddRequestRetransferTime();
                    //AddAndComputeRequestReTransferFrequency();
                    break;
                case MRUDPTransferReason::TRANSFER_FOR_TIMEOUT:
                    m_queue_array[dIndex]->AddTimeOutReTransferTime();
                    //AddAndComputeTimeoutReTransferFrequency();
                    break;
                default:
                    m_queue_array[dIndex]->AddTimeOutReTransferTime();
                    //AddAndComputeTimeoutReTransferFrequency();
                    break;
            }
        } else {
            printf("预期之外的数据\n");
            //AddAndComputeValidTransferFrequency();
        }
    }

    void RecvCircularDataQueue::NewDealSingleReceiveData(const std::shared_ptr<ReliableUdpDataReceiverStore> &pData) {
        const DWORD dIndex = pData->GetPacketIndex();
        const std::shared_ptr<ReliableUdpDataReceiverStore> &pTemp = m_queue_array[dIndex];    //取出目前队列内的该元素
        const DWORD &pDataCycleIndex = pData->GetCycleIndex();
        const DWORD &pTempCycleIndex = pTemp->GetCycleIndex();

        pthread_mutex_lock(&MutexOfQueIndexPtr_);

        printf("[MRUDPDebug]::CirculaerQ: NewDealSingleReceiveData\n");

        if ((pDataCycleIndex - pTempCycleIndex == 1) && !pTemp->IsAcked()) {
            //新来的包时效性较强，那么就把该包替换掉
            printf("[MRUDPDebug]::CirculaerQ: new comming packet and not acked..\n");
            m_queue_array[dIndex] = pData;    //注意这里千万不能把元素里面的数据部分置为nullptr
            BYTE transfer_reason = pData->GetTransferReason();
            switch (transfer_reason) {
                case MRUDPTransferReason::TRANSFER_FOR_INIT:
                    m_queue_array[dIndex]->SetSacked();
                    break;
                case MRUDPTransferReason::TRANSFER_FOR_REQUEST:
                    gMRUDPManager->mStatus.RealDealReTransmissionPackNum_SACK1++;
                    m_queue_array[dIndex]->AddRequestRetransferTime();
                    m_queue_array[dIndex]->SetSacked();
                    AddAndComputeRequestReTransferFrequency();
                    break;
                case MRUDPTransferReason::TRANSFER_FOR_TIMEOUT:
                    //
                    //printf("RealDealReTransmissionPackNum_TimeOut Add,预期外的错误\n");
                    gMRUDPManager->mStatus.RealDealReTransmissionPackNum_TimeOut1++;
                    m_queue_array[dIndex]->AddTimeOutReTransferTime();
                    m_queue_array[dIndex]->SetSacked();
                    AddAndComputeTimeoutReTransferFrequency();
                    break;
                default:
                    m_queue_array[dIndex]->AddTimeOutReTransferTime();
                    m_queue_array[dIndex]->SetSacked();
                    AddAndComputeTimeoutReTransferFrequency();
                    break;
            }
            //更改循环队列内容，如果dIndex在原有效队列外，还要把新加入队列的值设置为false，否则设置为true
            //isindexinvaild涉及到了線程的共享資源
            if (IsIndexInValid(dIndex)) {
                //如果在有效队列内，不需要做什么
                //printf("[MRUDPDebug]::收到了在有效队列内的序号为:%d的数据包,队列长度现在是:%d\n", dIndex, m_queue_size);
                AddAndComputeValidTransferFrequency();
            } else {
                //如果不在有效队列内，那么需要扩展队列，并从原tail（包括tail）起，把新增加入队列的元素（此元素除外）的收到标志置为false
                while (m_queue_tail != dIndex) {
                    SetElementNotReceiveByIndex(m_queue_tail);    //TODO 这里需要清空一些元素里的成员变量
                    m_queue_tail = (m_queue_tail + 1) % m_queue_capacity;
                    ++m_queue_size;
                }
                m_queue_tail = (dIndex + 1) % m_queue_capacity;
                //printf("[MRUDPDebug]::Index:%d,Cycle:%d Packet Input RecvQue\n", dIndex, pDataCycleIndex);
                ++m_queue_size;
                //重新标注索引
                /*printf("[MRUDPDebug]::收到了不在有效队列内的序号为:%d的数据包,拓展队列,队首为%d,队尾为%d,队列长度现在是:%d\n", dIndex,
                       m_queue_head, m_queue_tail, m_queue_size);*/
            }
        } else if (pDataCycleIndex == pTempCycleIndex) {
            //说明此数据包已经被接收，然而还没有被累积确认
            printf("[CCQ_R_CCQ:NewDealReceiveData]\n"\
                   "::Index:%d,Cycle:%d Packet recv RetransmissionPacket\n", dIndex, pDataCycleIndex);
            BYTE transfer_reason = pData->GetTransferReason();
            switch (transfer_reason) {
                case MRUDPTransferReason::TRANSFER_FOR_INIT:
                    break;
                case MRUDPTransferReason::TRANSFER_FOR_REQUEST:
                    gMRUDPManager->mStatus.RealDealReTransmissionPackNum_SACK2++;
                    m_queue_array[dIndex]->AddRequestRetransferTime();
                    AddAndComputeRequestReTransferFrequency();
                    break;
                case MRUDPTransferReason::TRANSFER_FOR_TIMEOUT:
                    gMRUDPManager->mStatus.RealDealReTransmissionPackNum_TimeOut2++;
                    m_queue_array[dIndex]->AddTimeOutReTransferTime();
                    AddAndComputeTimeoutReTransferFrequency();
                    break;
                default:
                    m_queue_array[dIndex]->AddTimeOutReTransferTime();
                    AddAndComputeTimeoutReTransferFrequency();
                    break;
            }
        } else {
            printf("[CCQ_R_CCQ:NewDealReceiveData] AddAndCompute...\n");
            AddAndComputeValidTransferFrequency();
            printf("[CCQ_R_CCQ:NewDealReceiveData] AddAndCompute...ok\n");
        }
        pthread_mutex_unlock(&MutexOfQueIndexPtr_);
    }

    void RecvCircularDataQueue::AddAndComputeRequestReTransferFrequency() {
        if (0 == m_recv_invalid_num_request) {
            m_recv_invalid_num_request_timestamp_start = GetTickCount();
        }
        m_recv_invalid_num_request++;
        if (m_recv_invalid_num_request >= STATISTIC_TIME) {
            DWORD time_used = GetTickCount() - m_recv_invalid_num_request_timestamp_start;
            m_recv_invalid_num_request_frequency = (m_recv_invalid_num_request * 1.0 / time_used) * 1000;
            /*printf("[MRUDPTransferTest]:Remote TID {%d} recv number request frequency: {%d}\n", m_d_remote_term_id,
                   m_recv_invalid_num_request_frequency);*/

            m_recv_invalid_num_request = 0;
            m_recv_invalid_num_request_timestamp_start = GetTickCount();
        }
    }

    void RecvCircularDataQueue::AddAndComputeTimeoutReTransferFrequency() {
        if (0 == m_recv_invalid_num_timeout) {
            m_recv_invalid_num_timeout_timestamp_start = GetTickCount();
        }
        m_recv_invalid_num_timeout++;
        if (m_recv_invalid_num_timeout >= STATISTIC_TIME) {
            DWORD time_used = GetTickCount() - m_recv_invalid_num_timeout_timestamp_start;
            m_recv_invalid_num_timeout_frequency = (m_recv_invalid_num_timeout * 1.0 / time_used) * 1000;

            m_recv_invalid_num_timeout = 0;
            m_recv_invalid_num_timeout_timestamp_start = GetTickCount();
        }
    }

    void RecvCircularDataQueue::AddAndComputeValidTransferFrequency() {
        if (0 == m_recv_invalid_num) {
            m_recv_invalid_num_timestamp_start = GetTickCount();
        }
        m_recv_invalid_num++;
        if (m_recv_invalid_num >= STATISTIC_TIME) {
            DWORD time_used = GetTickCount() - m_recv_invalid_num_timestamp_start;
            m_recv_invalid_num_frequency = (m_recv_invalid_num * 1.0 / time_used) * 1000;
            m_recv_invalid_num = 0;
            m_recv_invalid_num_timestamp_start = GetTickCount();
        }
    }

    std::shared_ptr<ReliableUdpDataReceiverStore> RecvCircularDataQueue::GetItemByIndex(DWORD dIndex) {
        if (dIndex >= 0 && dIndex <= m_queue_capacity - 1) {
            return m_queue_array.at(dIndex);
        }
        return nullptr;
    }

    bool RecvCircularDataQueue::OnRecvDataFromLowerLayers(const std::shared_ptr<ReliableUdpDataReceiverStore> &pData) {
        pthread_mutex_lock(&TempDataMutex_);
        m_queue_data_temp.push(pData);
        gMRUDPManager->mStatus.RealDealTransmissionDataPackNum++;
        printf("[MRUDPDebug]::Receive Data from LowerLayers, m_queue_data_temp size now is %lu\n", m_queue_data_temp.size());
        pthread_mutex_unlock(&TempDataMutex_);
        return true;
    }

    DWORD recv_data_count = 0;
    DWORD number_of_hundre = 0;


    std::shared_ptr<Force_ACK_Message> RecvCircularDataQueue::TraverseForceACK(DWORD dTermId) {
        if ((GetTickCount() - m_last_traverse_ack_time) > 1000) {
            //SPDLOG_LOGGER_WARN(MRUDPLogger, "START ACK TIME, {}, interval, {}", m_last_traverse_ack_time, (GetTickCount() - m_last_traverse_ack_time));
            /*printf("[MRUDPDebug]::接收队列的m_queue_size = %d,m_queue_head = %d, m_queue_capacity = %d\n", m_queue_size, m_queue_head,
                   m_queue_capacity);*/
        }
        m_last_traverse_ack_time = GetTickCount();
        //处理暂存数据接受队列中的数据
        DealReceiveData();//pass
        //printf("[MRUDPDebug]::接收完暂存数据后，接收队列的长度：%d\n", m_queue_size);
        //反馈给上层模块的本模块接收数据队列
        //std::deque<std::shared_ptr<ReliableUdpDataReceiverStore>> data_call_back_upper;
        std::shared_ptr<Force_ACK_Message> pFACK = nullptr; //最终返回的FACK;
        //std::shared_ptr<ReliableUdpDataReceiverStore> tail_packet = nullptr;
        {
            //pthread_mutex_lock(&Mutex_);
            if (0 == m_queue_size) {
                m_force_ack_message_we_send_last_time->SetTimeStamp(GetTickCount());
                pFACK = m_force_ack_message_we_send_last_time;
                /*if ((GetTickCount() - m_last_traverse_ack_time) > 1000) {
                    //SPDLOG_LOGGER_WARN(MRUDPLogger, "END ACK TIME, {}, interval, {}", GetTickCount(), (GetTickCount() - m_last_traverse_ack_time));
                }*/
                //pthread_mutex_unlock(&Mutex_);
                return pFACK;
            }
            //即将可能成为历史的有效队列大小
            const DWORD d_old_queue_size = m_queue_size;
            //从head遍历
            DWORD dTempIndex = m_queue_head;
            DWORD d_number_of_packet_have_vertify = 0;

            pthread_mutex_lock(&Mutex_);
            while (d_number_of_packet_have_vertify < d_old_queue_size) {
                std::shared_ptr<ReliableUdpDataReceiverStore> &pData = m_queue_array[dTempIndex];
                if (!pData->IsAcked()) {
                    //printf("[MRUDPDebug]::序号为%d的数据包没有被确认收到\n", dTempIndex);
                    //如果该元素没有收到,则直接跳出循环
                    break;
                } else {
                    d_number_of_packet_have_vertify++;
                    //向上层交付数据
                    data_call_back_upper.push_back(pData);
                    pData->SetUnAcked();
                    dTempIndex = (dTempIndex + 1) % m_queue_capacity;
                    m_queue_size--;
                }
            }
            pthread_mutex_unlock(&Mutex_);

            if (d_old_queue_size == m_queue_size) {
                //如果更新队列前后，队列大小没有变化，那么还是发送上一个FACK(更新时间戳
                m_force_ack_message_we_send_last_time->SetTimeStamp(GetTickCount());
                pFACK = m_force_ack_message_we_send_last_time;
                if ((GetTickCount() - m_last_traverse_ack_time) > 1000) {
                    /*SPDLOG_LOGGER_WARN(MRUDPLogger, "END ACK TIME, {}, interval, {}", GetTickCount(),
                                       (GetTickCount() - m_last_traverse_ack_time));*/
                }
                //printf("[MRUDPDebug]::更新队列后队列大小没有变化,还是发送上一个FACK(时间戳为:%d)\n", pFACK->GetTimeStamp());
                //pthread_mutex_unlock(&Mutex_);
                return pFACK;
            }

            //pthread_mutex_unlock(&Mutex_);

            //先更新有效队列
            m_queue_head = dTempIndex;
            //获得最后一个确认的数据包，注意不要直接减，防止出现负值
            dTempIndex = (dTempIndex + m_queue_capacity - 1) % m_queue_capacity;
            //一旦跳出，就可以生成FACK
            const std::shared_ptr<ReliableUdpDataReceiverStore> &tail_packet = m_queue_array[dTempIndex];
            //printf("[MRUDPDebug]::开始生成新FACK,最后一个连续确认的数据包index是%d,开始规划SACK区块\n", dTempIndex);
            std::vector<DWORD> sack_blocks;
            if (m_queue_size > 0 && m_queue_size != m_queue_capacity) {
                bool blocks_in = false;    //为true时表明正在遍历一个SACK区块
                for (DWORD index = m_queue_head; index != m_queue_tail; index = (index + 1) % m_queue_capacity) {
                    const std::shared_ptr<ReliableUdpDataReceiverStore> &pData = m_queue_array[index];
                    if (!pData->IsAcked()) {
                        //如果该元素没有收到，那么可能到了一个SACK区块的尾部
                        if (blocks_in) {
                            blocks_in = false;
                            sack_blocks.push_back((index + m_queue_capacity - 1) % m_queue_capacity);
                            //printf("[MRUDPDebug]::SACK区块尾:%d\n", (index + m_queue_capacity - 1) % m_queue_capacity);
                            if (sack_blocks.size() >= SACK_BLOCKS_MAX_NUM) {
                                break;
                            }
                        }
                    } else {
                        //如果该元素收到了，那么可能到了一个SACK区块的头部
                        if (!blocks_in) {
                            if (sack_blocks.size() >= SACK_BLOCKS_MAX_NUM) {
                                break;
                            }
                            blocks_in = true;    //进入SACK区块
                            sack_blocks.push_back(index);
                            //printf("[MRUDPDebug]::SACK区块头:%d\n", index);
                        }
                    }
                }
                if (blocks_in) {
                    //说明队列尾部的上一个元素是最后一个SACK区块的尾部
                    DWORD tmp = (m_queue_tail - 1 + m_queue_capacity) % m_queue_capacity;
                    sack_blocks.push_back(tmp);
                    //printf("[MRUDPDebug]::SACK区块尾:%d\n", tmp);
                    //sack_blocks.pop_back();
                }
            }
            pFACK = std::make_shared<Force_ACK_Message>(GetTickCount(), tail_packet, sack_blocks);
            //printf("[MRUDPDebug]::生成了新的FACK,SACK块长度为:%zu\n", pFACK->GetSackBlocks().size());
            m_force_ack_message_we_send_last_time = pFACK;
        }

        return pFACK;
    }

    void RecvCircularDataQueue::Update() {
        NewDealReceiveData();
    }

    //NewTraverseFACK和NewDealData之间要加锁，因为涉及到队列的状态，如que_size,que_head,que_tail等，或许可以改进Deal的方法，将队列状态的调整只放到一个函数里去
    std::shared_ptr<Force_ACK_Message> RecvCircularDataQueue::NewTraverseForceACK(DWORD dTermId) {
        //printf("[MRUDPDebug]::NewTraverseForceACK\n");
        std::shared_ptr<Force_ACK_Message> pFACK = nullptr; //最终返回的FACK;
        {
            pthread_mutex_lock(&MutexOfQueIndexPtr_);
            DWORD tempQueTail = m_queue_tail;
            DWORD tempQueSize = m_queue_size;
            if (tempQueSize == 0) {
                //printf("[MRUDPDebug]::No New ForceACK Generate\n");
                if (m_force_ack_message_we_send_last_time != nullptr) {
                    m_force_ack_message_we_send_last_time->SetTimeStamp(GetTickCount());
                }
                pthread_mutex_unlock(&MutexOfQueIndexPtr_);
                return m_force_ack_message_we_send_last_time;
            }

            DWORD d_number_of_packet_have_vertify = 0;

            pthread_mutex_lock(&Mutex_);
            while (d_number_of_packet_have_vertify < tempQueSize) {
                std::shared_ptr<ReliableUdpDataReceiverStore> &pData = m_queue_array[m_queue_head];
                if (!pData->IsAcked()) {
                    //如果该元素没有收到,则直接跳出循环
                    break;
                } else {
                    d_number_of_packet_have_vertify++;
                    //向上层交付数据
                    data_call_back_upper.push_back(pData);
                    pData->SetUnAcked();
                    m_queue_head = (m_queue_head + 1) % m_queue_capacity;
                    --tempQueSize;
                    //這裡在thread cunzai fengxian;
                    --m_queue_size;
                }
            }
            pthread_mutex_unlock(&Mutex_);

            //一旦跳出，就可以生成FACK
            DWORD dLastAckedIndex = (m_queue_head - 1 + m_queue_capacity) % m_queue_capacity;

            auto tail_packet = *m_queue_array[dLastAckedIndex].get();
            if ((tail_packet.GetCycleIndex() - m_queue_array[m_queue_head]->GetCycleIndex()) > 1) {
                tail_packet.SetCycleIndex(tail_packet.GetCycleIndex() - 1);
            }
            pthread_mutex_unlock(&MutexOfQueIndexPtr_);

            std::vector<DWORD> sack_blocks;
            if (tempQueSize > 0 && tempQueSize != m_queue_capacity) {
                bool blocks_in = false;    //为true时表明正在遍历一个SACK区块
                for (DWORD index = m_queue_head; index != tempQueTail; index = (index + 1) % m_queue_capacity) {
                    const std::shared_ptr<ReliableUdpDataReceiverStore> &pData = m_queue_array[index];
                    if (!pData->IsAcked()) {
                        //如果该元素没有收到，那么可能到了一个SACK区块的尾部
                        if (blocks_in) {
                            blocks_in = false;
                            sack_blocks.push_back((index + m_queue_capacity - 1) % m_queue_capacity);
                            //printf("[MRUDPDebug]::SACK区块尾:%d\n", (index + m_queue_capacity - 1) % m_queue_capacity);
                            if (sack_blocks.size() >= SACK_BLOCKS_MAX_NUM) {
                                break;
                            }
                        }
                    } else {
                        //如果该元素收到了，那么可能到了一个SACK区块的头部
                        if (!blocks_in) {
                            if (sack_blocks.size() >= SACK_BLOCKS_MAX_NUM) {
                                break;
                            }
                            blocks_in = true;    //进入SACK区块
                            sack_blocks.push_back(index);
                            //printf("[MRUDPDebug]::SACK区块头:%d\n", index);
                        }
                    }
                }
                if (blocks_in) {
                    //说明队列尾部的上一个元素是最后一个SACK区块的尾部
                    DWORD tmp = (tempQueTail - 1 + m_queue_capacity) % m_queue_capacity;
                    sack_blocks.push_back(tmp);
                    //printf("[MRUDPDebug]::SACK区块尾:%d\n", tmp);
                    //sack_blocks.pop_back();
                }
            }
            pFACK = std::make_shared<Force_ACK_Message>(GetTickCount(), tail_packet, sack_blocks);
        }

        m_force_ack_message_we_send_last_time = pFACK;
        return pFACK;
    }

    void RecvCircularDataQueue::UploadDataPacket(DWORD dTermId) {
        pthread_mutex_lock(&Mutex_);
        auto uploadqueue = data_call_back_upper;
        //printf("!!!![MRUDPDebug]::StartUploadData:(uploadqueue size:%d)\n", uploadqueue.size());
        data_call_back_upper.clear();
        pthread_mutex_unlock(&Mutex_);
        while (!uploadqueue.empty()) {

            //统计数据包数量
            std::shared_ptr<ReliableUdpDataReceiverStore> &p_data_temp = uploadqueue.front();

            printf("_________________________\n");
            printf("+++++++++++++++++++++++++\n");
            printf("^^^^^^^^^^^^^^^^^^^^^^^^^\n");
            printf("MRUDP:recv reliable data ...\n");
            printf("Check Data type:\n");
            //检查数据类型，如果不是数据服务类型数据，则交给BigDataTransfer的回调
            if(dataserver_check(p_data_temp->GetData(), p_data_temp->GetDataLength())==false){
                printf("CallBackDataFunc => BigDataTransfer\n");
                CallBackDataFunc(dTermId, p_data_temp->GetData(), 
                                 p_data_temp->GetDataLength());
                printf("...CallBackDataFunc...ok\n");
            }
            else { //如果是数据服务类型数据，则交给DATA SERVER的回调
                //MRUDP_DATASERVER_CALLBACK
                //数据回调服务对应的回调调用 240910-houlc spin_server_callback
                printf("spin_server_callback => DATA SERVER\n");
                bool spin_res = spin_server_callback(dTermId,
                                p_data_temp->GetData(),
                                p_data_temp->GetDataLength()) ;
                printf("...spin_server_callback...ok(res:%d)\n", spin_res);
            }
            printf("#########################\n");

            uploadqueue.pop_front();
        }
    }

    /**
     * 不包括队尾元素
     */
    bool RecvCircularDataQueue::IsIndexInValid(DWORD dIndex) const {
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

    bool RecvCircularDataQueue::NewIsIndexInValid(DWORD dIndex) const {

        if (m_queue_head > m_queue_tail) {
            if (dIndex >= m_queue_tail && dIndex < m_queue_head) { return false; }
            else { return true; }
        }

        if (m_queue_head < m_queue_tail) {
            if (dIndex >= m_queue_head && dIndex < m_queue_tail) { return true; }
            else { return false; }
        }

        if (m_queue_head == m_queue_tail) {
            if (m_queue_array[m_queue_head]->IsAcked()) {
                return true;
            } else {
                return false;
            }
        }
        return true;
    }
}
