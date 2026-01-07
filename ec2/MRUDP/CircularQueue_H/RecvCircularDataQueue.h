//
// Created by 王炳棋 on 2022/12/24.
//

#include "IniHandleApi.h"
#include "Transfer_H/ReliableUdpMessage.h"
#include "AbstractStorage_H/AbstractStorage.h"
#include "AbstractStorage_H/MemoryPartStorage.h"

#ifndef NETCOMBTRANSFER_RECVCIRCULARDATAQUEUE_H
#define NETCOMBTRANSFER_RECVCIRCULARDATAQUEUE_H


namespace MRUDP {
    class RecvCircularDataQueue {
    public:
        RecvCircularDataQueue(DWORD dSelfTermId, DWORD dRemoteTermId) {
            m_queue_capacity = RUDP_CYCLEFILE_CAPACITY;
            //用vector开辟数组
            m_queue_array.resize(m_queue_capacity);
            m_storage = std::make_shared<MemoryStorageSender>(m_queue_capacity);//TODO 按照配置文件配置存取机制
            //获取配置文件项
            //m_d_scan_next_queue_head_for_retransfer = 1;
            m_d_scan_next_queue_head_for_retransfer = GetIntegerKeyIni("MRUDP", "ScanNextQueueHeadForRetransfer", 50);
            m_d_timestamp_diff_threshold = GetIntegerKeyIni("MRUDP", "TimestampDiffThreshold", 300);
            m_d_self_term_id = dSelfTermId;
            m_d_remote_term_id = dRemoteTermId;

            //初始化数组
            for (DWORD i = 0; i < m_queue_capacity; i++) {
                m_queue_array[i] = std::make_shared<ReliableUdpDataReceiverStore>();
            }

            pthread_mutex_init(&Mutex_, nullptr);
            pthread_mutex_init(&TempDataMutex_, nullptr);
            pthread_mutex_init(&MutexOfQueIndexPtr_, nullptr);
            //printf("End2EndTransmission_RecvQueueInit_Success with size:%d\n", m_queue_capacity);
        }

        virtual ~RecvCircularDataQueue() {
            pthread_mutex_destroy(&Mutex_);
            pthread_mutex_destroy(&TempDataMutex_);
            pthread_mutex_destroy(&MutexOfQueIndexPtr_);
        }

    private:
        DWORD m_last_traverse_ack_time;    //调用生成ACK函数的时间戳
        DWORD m_queue_capacity = 0;    //循环队列的容量
        DWORD m_queue_head = 0;            //循环队列当下的队首索引
        DWORD m_queue_tail = 0;            //循环队列当下的队尾索引
        DWORD m_queue_size = 0;                //循环队列当下有效的元素数目
        std::vector<std::shared_ptr<ReliableUdpDataReceiverStore>> m_queue_array;    //用来存储循环队列数据的数组
        std::deque<std::shared_ptr<ReliableUdpDataReceiverStore>> data_call_back_upper;
        std::shared_ptr<AbstractStorage> m_storage;    //用来随机存取数据
        pthread_mutex_t Mutex_;            //用来进行临界区控制
        DWORD m_d_scan_next_queue_head_for_retransfer = 0;
        //如果扫描到的重传包与收到FACK时的包相差超过此阈值则重传（毫秒）
        DWORD m_d_timestamp_diff_threshold = GetIntegerKeyIni("MRUDP", "TIMESTAMP_DIFF_THRESHOLD", 50);

        DWORD m_d_self_term_id = 0;    //本端ID
        DWORD m_d_remote_term_id = 0; //对端ID
        //存储本端作为接收端发送的上一个ACK
        std::shared_ptr<Force_ACK_Message> m_force_ack_message_we_send_last_time = std::make_shared<Force_ACK_Message>();

        const DWORD RUDP_CYCLEFILE_CAPACITY = GetIntegerKeyIni("MRUDP", "RUDP_CYCLEFILE_CAPACITY", 1000);
        //const DWORD SACK_BLOCKS_MAX_NUM = 4 * 2;
        const DWORD SACK_BLOCKS_MAX_NUM = GetIntegerKeyIni("MRUDP", "RUDP_SACK_BLOCKS_MAX_NUM", 3) * 2;
        pthread_mutex_t TempDataMutex_;
        std::queue<std::shared_ptr<ReliableUdpDataReceiverStore>> m_queue_data_temp; //暂存接收到的数据


        pthread_mutex_t MutexOfQueIndexPtr_;

        void DealReceiveData(); //处理暂存接受队列里的数据
        void NewDealReceiveData();

        void DealSingleReceiveData(const std::shared_ptr<ReliableUdpDataReceiverStore> &pData);   //处理暂存数据接收队列中的单个元素
        void NewDealSingleReceiveData(const std::shared_ptr<ReliableUdpDataReceiverStore> &pData);

        DWORD m_recv_invalid_num_timeout = 0;
        DWORD m_recv_invalid_num_timeout_timestamp_start = 0;
        DWORD m_recv_invalid_num_timeout_frequency = 0;
        DWORD m_recv_invalid_num_request = 0;
        DWORD m_recv_invalid_num_request_timestamp_start = 0;
        DWORD m_recv_invalid_num_request_frequency = 0;

        //DWORD STATISTIC_TIME = 1000;
        DWORD STATISTIC_TIME = GetIntegerKeyIni("Main", "TestCycle", 1000);

        void AddAndComputeRequestReTransferFrequency();

        void AddAndComputeTimeoutReTransferFrequency();

        DWORD m_recv_invalid_num = 0;
        DWORD m_recv_invalid_num_timestamp_start = 0;
        DWORD m_recv_invalid_num_frequency = 0;

        void AddAndComputeValidTransferFrequency();

    public:
        void Update();

        void UploadDataPacket(DWORD dTermId);

        bool OnRecvDataFromLowerLayers(const std::shared_ptr<ReliableUdpDataReceiverStore> &pData);

        /**
		 * 返回第dIndex索引对应的字节序列，及其长度dDataLength
		 */
        std::shared_ptr<BYTE> SendDataByIndex(const DWORD dIndex, DWORD &dDataLength);

        /**
		 * 将接收队列里的数据交付给上层模块，并生成FACK
		 */
        std::shared_ptr<Force_ACK_Message> TraverseForceACK(DWORD dTermId);

        /*
         * new ack generate Func without DealTempData, ACK generate quickly, low delay control pass
         * */
        std::shared_ptr<Force_ACK_Message> NewTraverseForceACK(DWORD dTermId);

        DWORD GetQueueStatus() const {
            return m_queue_size;
        }

    private:
        /**
		 * 从数组中按照索引返回某个元素
		 */
        std::shared_ptr<ReliableUdpDataReceiverStore> GetItemByIndex(DWORD dIndex);

        /**
		 * 根据接收到的FACK设置数据发送队列队首元素的索引，dIndex必须在循环队列的有效范围内
		 * 此函数旨在缩小循环队列有效范围的大小
		 */
        bool SetHeadIndex(std::shared_ptr<const Force_ACK_Message> pFackMessage);

        /**
		 * 在遍历本地接收队列生成ACK时使用，更改数据接收队列的索引
		 */
        bool SetHeadIndex(DWORD dIndex);
        /**
		 * 直接设置队尾元素的索引，dIndex必须不在循环队列的有效范围内或者与前队尾元素索引相等，但是必须在循环队列的可能范围内（由容量决定）
		 * 此函数旨在扩大循环队列有效范围的大小，譬如当做数据接收循环队列时接收到对端发来的新数据包
		 */
        //bool SetTailIndex(DWORD32 dIndex);

        /**
         * 按照参数的索引值，将相应的元素放入数组
         */
        /**
         * 按照参数的索引值，将相应的元素放入数组
         */
        bool SetItemByIndex(DWORD dIndex, std::shared_ptr<ReliableUdpDataReceiverStore> pItem);

        /**
		 * 如果参数在队列范围内（不包括tail序号），返回true，否则返回false
		 */
        bool IsIndexInValid(DWORD dIndex) const;

        bool NewIsIndexInValid(DWORD dIndex) const;

        DWORD GetTailIndex() const {
            return m_queue_tail;
        }

        void SetElementNotReceiveByIndex(DWORD dIndex) {
            m_queue_array[dIndex]->SetUnAcked();
        }

    private:
        /**
         * 内部工具函数，判断循环队列队首队尾索引的状态，注意此函数并不是线程安全的，只用作内部使用
         */
        CircularDataQueueIndexStatus GetStatusValidQueue() const;
    };
}

#endif //NETCOMBTRANSFER_RECVCIRCULARDATAQUEUE_H
