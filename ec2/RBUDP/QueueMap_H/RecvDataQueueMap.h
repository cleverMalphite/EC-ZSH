//
// Created by Kong on 2023/6/9.
//

#ifndef RBUDP_RECVDATAQUEUEMAP_H
#define RBUDP_RECVDATAQUEUEMAP_H


#include "vector"
#include "Transfer_H/NAK_Message.h"
#include "Transfer_H/RBUDPdata.h"
#include "queue"
#include "map"
#include "set"
#include "algorithm"
#include "AbstractStorage_RBUDP_H//FileStorage.h"
#include "../IniHandle/IniHandleApi.h"
#include "Store_H/RBUDPDataReceiverStore.h"

namespace RBUDP
{
    //该类主要是用于实现接收缓冲区的存放作用，当这里面的数据被放到这里面我们认为被接受，每当生成相应的NAK之后我们就将这些数据上传或者存放到某一个介质中并将队列中的数据删除

    class RecvDataQueueMap
    {
    public:
        RecvDataQueueMap(DWORD dSelfTermInd, DWORD dRemoteTermId) {

            m_file_path = GetStringValueKeyIni("RBUDP", "RBUDP_RECV_TEMP_FILEPATH", "");
            m_file_segmatic_size = 1369;

            m_storage = std::make_shared<FileStorage>(m_file_path,m_file_segmatic_size);

            //m_queue_capacity = RBUDP_CYCLEFILE_CAPACITY;

            //m_storage = make_shared<MemoryStorageSender>(m_queue_capacity);

            pthread_mutex_init(&m_cs, nullptr);
            pthread_mutex_init(&m_data_tmp_queue_cs, nullptr);

        }

        ~RecvDataQueueMap() {
            pthread_mutex_destroy(&m_cs);
            pthread_mutex_destroy(&m_data_tmp_queue_cs);
        }


    private:
        DWORD m_queue_capacity = 0;
        DWORD m_queue_size = 0;
        DWORD m_file_size = 0;
        DWORD m_recv_count = 2280;
        std::shared_ptr<AbstractStorage> m_storage;	//用来随机存取数据
        //std::vector<std::shared_ptr<RBUDPDataReceiverStore> > m_queue_array;
        std::deque<std::shared_ptr<RBUDPdata>> data_callback_upper;
        std::set<DWORD> m_queue_map;			//用于存储目前已经接收到的数据的序号的数据队列
        std::set<DWORD> m_send_queue;			//用于存储需要发送的已经接收到的数据序号
        std::map<DWORD,std::shared_ptr<RBUDPdata>> m_queue_data_tmp;				//暂存接收到的数据
        pthread_mutex_t m_cs;
        pthread_mutex_t m_data_tmp_queue_cs;
        void DealSingleReceiveData(std::shared_ptr<RBUDPdata> data);	//处理暂存数据接收队列中的单个元素
        //shared_ptr<NAK_Message>m_nak_message_send_last_time = make_shared<NAK_Message>();		//存储作为接收端上一次发送的NAK
        //DWORD min_update_count = 3000;		//用于管理最少多少数量的连续数据包可以提交
        DWORD m_head_index = 0;			//当前需要上传的索引
        DWORD m_tail_index = 0;			//上次结束时删除的索引尾
        std::string m_file_path;
        int m_file_segmatic_size = 1369;
        //TODO

        std::vector<std::shared_ptr<NAK_Message>> tmp_NAK;

    public:
        void UploadDataPacket(DWORD dTermId);

        bool OnRecvDataFromLowerLayers(std::shared_ptr<RBUDPdata> p_data);

        bool OnRecvIndexFromLowerLayers(std::shared_ptr<RBUDPdata> p_data);

        std::shared_ptr<NAK_Message> TraverseNAK(DWORD dTermId);
        std::shared_ptr<NAK_Message> TraverseNAK_WithoutSeg(DWORD dTermId);


        bool AddItem(std::shared_ptr<RBUDPdata> p_item);				//用于向存储介质中插入数据部分，与序号的插入分开
        bool AddIndex(std::shared_ptr<RBUDPdata> p_index);			//用于向两个队列中插入序号部分

        DWORD ComputeAvg();

        DWORD ComputeSeg();

        bool ComputeLoss();

        bool ComputeGap(DWORD iter, DWORD avg);
    };
}

#endif //RBUDP_RECVDATAQUEUEMAP_H
