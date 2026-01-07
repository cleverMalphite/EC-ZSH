//
// Created by Kong on 2023/6/9.
//

#ifndef RBUDP_SENDDATAQUEUEMAP_H
#define RBUDP_SENDDATAQUEUEMAP_H



#include "vector"
#include "AbstractStorage_RBUDP_H//FileStorage.h"
#include "Transfer_H/NAK_Message.h"
#include "../../NetCombTransfer/CNetTerminalManager.h"
#include "Store_H/RBUDPDataSenderStore.h"
#include "RBUDPApi.h"
#include "map"
#include "set"

namespace RBUDP
{
    class SendDataQueueMap
    {
    public:
        SendDataQueueMap(DWORD dSelfTermId, DWORD dRemoteTermId) {
            m_d_self_term_id = dSelfTermId;
            m_d_remote_term_id = dRemoteTermId;
            m_file_path = GetStringValueKeyIni("RBUDP", "RBUDP_SEND_TEMP_FILEPATH", "");
            //m_file_segmatic_size = GetIntegerKeyIni("RBUDP", "RBUDP_FILESIZE", 1369);
            m_file_segmatic_size = 1369;

            //m_queue_capacity = RBUDP_CYCLEFILE_CAPACITY;
            m_storage = std::make_shared<FileStorage>(m_file_path, m_file_segmatic_size);
            //m_storage = make_shared<MemoryStorageSender>(m_queue_capacity);
            pthread_mutex_init(&m_cs, nullptr);
            pthread_mutex_init(&m_data_tmp_queue_cs, nullptr);
            //TODO
        }

        virtual ~SendDataQueueMap()
        {
            pthread_mutex_destroy(&m_cs);
            pthread_mutex_destroy(&m_data_tmp_queue_cs);
        }

    private:
        DWORD m_queue_capacity = 0;
        DWORD m_map_size = 0;				//当前映射中有效元素数目
        DWORD m_last_confirm_time = 0;	//上次NAK时间
        DWORD m_last_traverse_nak_time;	//当前表示调用生成NAK函数的时间戳，TODO等到以后还要在添加使用ack生成函数的时间戳
        DWORD m_map_capacity = 0;			//当前存放映射最大值
        DWORD m_map_index = 0;			//当前放入的文件元素序列号，用以插入
        DWORD m_tail_index = 0;		//存放发送端发送的数据的最大索引
        std::vector<DWORD> timeout_queue;		//用于根据NAK得到丢失的数据包索引，用于数据发送
        DWORD m_d_self_term_id = 0;	//本端ID
        DWORD m_d_remote_term_id = 0; //对端ID
        std::shared_ptr<AbstractStorage> m_storage;	//用来随机存取数据
        std::map<DWORD, std::shared_ptr<RBUDPDataSenderStore> > m_queue_map; //用于存储队列映射数据
        const DWORD RBUDP_CYCLEFILE_CAPACITY = GetIntegerKeyIni("MRUDP", "RUDP_CYCLEFILE_CAPACITY", 1000);
        DWORD m_d_retransmission_time=50;							//用于设置超时重传检查时间
        std::string m_file_path;			//用于文件暂存的绝对路径
        int m_file_segmatic_size = 0;
        DWORD rest_retransfer = 0;
        bool timeout_control = false;		//控制重传检测，ps此方法相当于忽视超时重传
        DWORD last_timeout_time = GetTickCount();


        pthread_mutex_t m_cs;
        pthread_mutex_t m_data_tmp_queue_cs;						//暂存接收数据 的临界区

    public:
        //向映射中添加一个元素，如果成功返回true
        bool AddItem(std::shared_ptr<RBUDPDataSenderStore> p_item, DWORD filesize);

        bool AddItemInStorage(std::shared_ptr<RBUDPDataSenderStore> p_item, DWORD FileSize);

        bool SetMaxIndex(DWORD index);

        bool AddItemtoTimeout(DWORD dIndex);

        //返回第dIndex索引对应的字节序列，及其长度dDataLength
        std::shared_ptr<BYTE> SendDataByIndex(const DWORD dIndex, DWORD& dDataLength) { return m_storage->GetDataByIndex(dIndex, dDataLength); }

        /**
         * 处理接收到的FACK，返回重发的数据包数目
         */
        DWORD OnRecvMessageFromRecvTerm(std::shared_ptr<NAK_Message> force_ack_message);

        DWORD SendDataInSacks();

        std::set<DWORD> GetIndexInSacks(std::shared_ptr<NAK_Message> nak_message);

        int DeleteIndexInMap(std::shared_ptr<NAK_Message> nak_message);

        //循环处理超时数据
        bool DealTimeOut();
        bool IsEmpty();

        //TODO
    };
}
#endif //RBUDP_SENDDATAQUEUEMAP_H
