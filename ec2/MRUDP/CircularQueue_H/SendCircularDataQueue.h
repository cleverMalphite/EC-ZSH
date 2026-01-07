//
// Created by 王炳棋 on 2022/12/23.
//

#include <vector>
#include "../../NetCombTransfer/CNetTerminalManager.h"
#include "Transfer_H/ReliableUdpMessage.h"
#include "AbstractStorage_H/AbstractStorage.h"
#include "AbstractStorage_H/MemoryPartStorage.h"
#include "Store_H/ReliableUdpDataReceiverStore.h"

#ifndef NETCOMBTRANSFER_SENDCIRCULARDATAQUEUE_H
#define NETCOMBTRANSFER_SENDCIRCULARDATAQUEUE_H

namespace MRUDP {
    class SendCircularDataQueue {
    public:
        SendCircularDataQueue(DWORD dSelfTermId, DWORD dRemoteTermId) :
                m_d_self_term_id(dSelfTermId), m_d_remote_term_id(dRemoteTermId) {
            m_queue_windows_expect_num = WINDOWS_EXPECT_NUM_MIN;    //流量控制窗口期望大小，应初始为循环队列大小
            m_queue_capacity = RUDP_CYCLEFILE_CAPACITY;
            m_queue_array.resize(m_queue_capacity);
            m_storage = std::make_shared<MemoryStorageSender>(m_queue_capacity);
            //m_d_scan_next_queue_head_for_retransfer = 50;
            m_d_scan_next_queue_head_for_retransfer = GetIntegerKeyIni("MRUDP", "ScanNextQueueHeadForRetransfer", 50);
            //初始化数组
            for (DWORD i = 0; i < m_queue_capacity; i++) {
                m_queue_array[i] = std::make_shared<ReliableUdpDataSenderStore>();
            }
            pthread_mutex_init(&Mutex_, nullptr);
            pthread_mutex_init(&TempDataMutex_, nullptr);
            //printf("End2EndTransmission_SendQueueInit_Success with size %d\n", m_queue_capacity);
        }

        virtual ~SendCircularDataQueue() {
            pthread_mutex_destroy(&Mutex_);
            pthread_mutex_destroy(&TempDataMutex_);
        }

    private:
        DWORD m_queue_capacity = 0;     //循环队列容量
        DWORD m_last_confirm_time = 0;  //上一次ack有效时间
        DWORD m_dCycleIndex = 1;        //包传输轮次,0~65535为一个轮次
        DWORD m_queue_windows_expect_num = 0;//流量控制窗口中元素数目的期望最大值，用来做流量控制
        DWORD m_queue_head = 0;         //循环队列当前队首索引
        DWORD m_queue_tail = 0;         //循环队列当前队尾索引
        DWORD m_queue_size = 0;         //循环队列当前有效元素数目
        DWORD m_recv_ack_invalid_number = 0;//收到无效ACK的数目，为[0,2]的值，循环增加，为2时m_queue_windows_expect_num减半
        const DWORD RECV_ACK_INVALID_MAX_COUNT_NUM = 0;//m_recv_ack_invalid_number的最大值
        const DWORD WINDOWS_EXPECT_NUM_MIN = 20000;//流量控制窗口的最小值；
        const float WINDOWS_EXPECT_NUM_MULIT_WHEN_LOSS_DETECTED = 0.75;//流量窗口缩小所乘值
        /*const float WINDOWS_EXPECT_NUM_MULIT_WHEN_LOSS_DETECTED = GetRealValueKeyIni("MRUDP", "WINDOWS_EXPECT_NUM_MULTI_WHEN_LOSS_DETECTED", 0.75);*/
        std::vector<std::shared_ptr<ReliableUdpDataSenderStore>> m_queue_array; //用来存储循环队列数据的数组
        const static DWORD time_threshold_change_packets_num = 200;//每time_threshold_change_packets_num个数据包，将时间阈值增加time_threshold_change毫秒
        const static DWORD time_threshold_change = 200;//每多少个数据包，将时间阈值增加time_threshold_change毫秒
        std::shared_ptr<AbstractStorage> m_storage;
        pthread_mutex_t Mutex_;
        DWORD m_d_scan_next_queue_head_for_retransfer = 0;
        //DWORD m_d_timestamp_diff_threshold = 50; //如果扫描到的重传包与收到FACK时的包相差超过此阈值则重传（毫秒）
        DWORD m_d_timestamp_diff_threshold = GetIntegerKeyIni("MRUDP", "TIMESTAMP_DIFF_THRESHOLD", 50);
        //DWORD TIME_THRESHOLD = 50;
        DWORD TIME_THRESHOLD = GetIntegerKeyIni("MRUDP", "TIMESTAMP_DIFF_THRESHOLD", 50);
        DWORD m_d_self_term_id = 0;   //本端ID
        DWORD m_d_remote_term_id = 0;   //对端ID
        std::shared_ptr<Force_ACK_Message> m_force_ack_message_we_send_last_time = std::make_shared<Force_ACK_Message>();//存储本端作为接收端发送的上一个ACK
        //const DWORD RUDP_CYCLEFILE_CAPACITY = 40000;
        const DWORD RUDP_CYCLEFILE_CAPACITY = GetIntegerKeyIni("MRUDP", "RUDP_CYCLEFILE_CAPACITY", 1000);

        pthread_mutex_t TempDataMutex_;
        DWORD m_send_num_timeout = 0;   //两次生成FACK间隔内接收端接收到的，发送端因为超时而发送的包数目
        DWORD m_send_num_timeout_timestamp_start = 0;//每个统计周期的开始的时间戳
        DWORD m_send_num_timeout_frequency = 0;//发送端因为超时而发送的包频率
        DWORD m_send_num_request = 0;  //两次生成FACK间隔内接收端接收到的，发送端因为请求重传而发送的包数目
        DWORD m_send_num_request_timestamp_start = 0;    //每个统计周期的开始的时间戳
        DWORD m_send_num_request_frequency = 0; //两次生成FACK间隔内接收端接收到的，发送端因为请求重传而发送的包频率
        //DWORD STATISTIC_TIME = 1000;    //统计周期
        DWORD STATISTIC_TIME = GetIntegerKeyIni("Main", "TestCycle", 1000);

        void AddAndComputeRequestReTransferFrequency(DWORD num);

        void AddAndComputeTimeoutReTransferFrequency(DWORD num);

    public:
        /**
		 * 向数组中添加一个元素，如果成功添加则返回true，否则返回false
		 */
        bool AddItem(const std::shared_ptr<ReliableUdpDataSenderStore>& pItem);

        /**
		 * 处理接收到的FACK，返回重发的数据包数目
		 */
        DWORD OnRecvMessageFromRecvTerm(const std::shared_ptr<Force_ACK_Message>& force_ack_message);

        /**
		 * 返回第dIndex索引对应的字节序列，及其长度dDataLength
		 */
        std::shared_ptr<BYTE> SendDataByIndex(const DWORD dIndex, DWORD &dDataLength) {
            return m_storage->GetDataByIndex(dIndex, dDataLength);
        }

        DWORD GetQueueSize() const {
            return m_queue_size;
        }

        bool SendAllUnACKPackets();

    private:
        //发送从[firstIndex, lastIndex]间所有超过时间阈值的元素,返回发送元素的数目
        DWORD SendDataInSacks(DWORD firstIndex, DWORD lastIndex);

        /**
		 * 从数组中按照索引返回某个元素
		 */
        std::shared_ptr<ReliableUdpDataSenderStore> GetItemByIndex(DWORD dIndex);

        /**
		 * 根据接收到的FACK设置数据发送队列队首元素的索引，dIndex必须在循环队列的有效范围内
		 * 此函数旨在缩小循环队列有效范围的大小
		 */
        bool SetHeadIndex(const std::shared_ptr<const Force_ACK_Message>& pFackMessage);
        bool NewSetHeadIndex(const std::shared_ptr<const Force_ACK_Message>& pFackMessage);
        /**
		 * 在遍历本地接收队列生成ACK时使用，更改数据接收队列的索引
		 */
        bool SetHeadIndex(DWORD dIndex);

        /**
         * 按照参数的索引值，将相应的元素放入数组
         */
        bool SetItemByIndex(DWORD dIndex, std::shared_ptr<ReliableUdpDataSenderStore> pItem);

        /**
		 * 如果参数在队列范围内（不包括tail序号），返回true，否则返回false
		 */
        bool IsIndexInValid(DWORD dIndex) const;

        DWORD GetTailIndex() const { return m_queue_tail; }

        DWORD Retransfer_UnAcked_Packets_in_SendWindow(bool isUrgentTransfer, DWORD &d_resend_number,
                                                       DWORD &d_retransfer_for_request, DWORD &d_retransfer_for_timeout);

    private:
        /**
		 * 内部工具函数，判断循环队列队首队尾索引的状态，注意此函数并不是线程安全的，只用作内部使用
		 */
        CircularDataQueueIndexStatus GetStatusValidQueue() const;

    };
}

#endif //NETCOMBTRANSFER_SENDCIRCULARDATAQUEUE_H
