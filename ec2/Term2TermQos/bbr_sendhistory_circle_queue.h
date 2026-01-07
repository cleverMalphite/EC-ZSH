#pragma once
//
#include <vector>
#include "../MRUDP/AbstractStorage_H/AbstractStorage.h"

#include "bbr_skiplist.h"
#include "bbr_circlequeue.h"
#include "../Util/LockGuard.h"
#include "../NetCombTransfer/CNetTerminalManager.h"
#include "../MRUDP/AbstractStorage_H/MemoryPartStorage.h"
#include "../SpeedControl/QosStructInterfaceInfo.h"
/**
 * 用来标志循环队列队首队尾元素状态
 */
//enum Qos_QOS_CircularDataQueueIndexStatus
//{
//	/* 分以下几种情况：
//	* 1. 队列首索引比队列尾索引小；
//	* 2. 队列首索引比队列尾索引大；
//	* 3. 队列首索引与队列尾索引相等，且队列有效长度为0；
//	* 4. 队列首索引与队列尾索引相等，且队列有效长度为队列容量大小；
//	* 5. 索引错误状态（不属于上述状态之一）
//	*/
//	QOS_Queue_Head_Index_Less_Than_Queue_Tail_Index,
//	QOS_Queue_Head_Index_Greater_Than_Queue_Tail_Index,
//	QOS_Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Full,
//	QOS_Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Zero,
//	QOS_Wrong_Head_Tail_Index_Status
//};

/**
 * 这是一个循环队列类
 */
class SendCircularDataQueue
{
public:
    SendCircularDataQueue(/*DWORD capacity*//*DWORD dSelfTermId, DWORD dRemoteTermId*/)
    {
        m_sendpacketsqueue_capacity = QOS_CYCLEFILE_CAPACITY;
        //用vector开辟数组
        m_sendpacketsqueue_array.resize(m_sendpacketsqueue_capacity);
        //m_storage = make_shared<MemoryStorage>(m_sendpacketsqueue_capacity);//TODO 按照配置文件配置存取机制
        //获取配置文件项
        //初始化数组
        for (DWORD i = 0; i < m_sendpacketsqueue_capacity; i++)
        {
            m_sendpacketsqueue_array[i] = std::make_shared<bbr_packet_point_t>();
            m_sendpacketsqueue_array[i]->send_time = 0;
            m_sendpacketsqueue_array[i]->size = 0;

            m_sendpacketsqueue_array[i]->total_data_sent = 0;
            m_sendpacketsqueue_array[i]->total_data_acked_at_the_last_recv_fbpacket= 0;
            m_sendpacketsqueue_array[i]->total_data_sent_at_last_acked_packet = 0;

            m_sendpacketsqueue_array[i]->last_acked_packet_ack_time = -1;
            m_sendpacketsqueue_array[i]->last_acked_packet_sent_time = -1;
            m_sendpacketsqueue_array[i]->is_app_limited = 0;
            m_sendpacketsqueue_array[i]->ignore = 1;
        }
        pthread_mutex_init(&m_cs, nullptr);
        //InitializeCriticalSection(&m_data_tmp_queue_cs);
    };

    virtual ~SendCircularDataQueue()
    {
        pthread_mutex_destroy(&m_cs);
        //DeleteCriticalSection(&m_data_tmp_queue_cs);
    }
    DWORD m_sendpacketsqueue_size = 0;				//循环队列当下有效的元素数目
private:
    DWORD m_sendpacketsqueue_capacity = 0;	//循环队列的容量
    DWORD m_sendpacketsqueue_head = 0;			//循环队列当下的队首索引
    DWORD m_sendpacketsqueue_tail = 0;			//循环队列当下的队尾索引
    DWORD m_sendpacketsqueue_tail_tmp;
    std::vector<shared_ptr<bbr_packet_point_t> > m_sendpacketsqueue_array;	//用来存储循环队列数据的数组
    //shared_ptr<AbstractStorage> m_storage;	//用来随机存取数据
    pthread_mutex_t m_cs;			//用来进行临界区控制
    const DWORD QOS_CYCLEFILE_CAPACITY = 65530;

public:
    /**
     * 向数组中添加一个元素，如果成功添加则返回true，否则返回false
     */
    bool QosSend_AddItem(shared_ptr<bbr_packet_point_t> p_item);
    //void Qos_DealSingleReceiveData(shared_ptr<const RemoteToSelfQosInfo> data);	//处理暂存数据接收队列中的单个元素
    //bool QosSend_SetHeadIndex(DWORD dHeaderIndex);
    DWORD QosSend_GetTailIndex() { return m_sendpacketsqueue_tail; }
    DWORD QosSend_SetPosition(DWORD pos);
    DWORD QosSend_GetHeadIndex() { return m_sendpacketsqueue_head; }
    bool QosSend_SetHeadIndexStepByStep(DWORD dHeaderIndex);
    shared_ptr<bbr_packet_point_t> QosSend_GetItemByIndex(DWORD dIndex);
    bool QosSend_IsIndexInValid(DWORD dIndex);

    /**
     * 处理接收到的FACK，返回重发的数据包数目
     */
    //DWORD OnRecvMessageFromRecvTerm(shared_ptr<Force_ACK_Message> force_ack_message);

    //bool OnRecvDataFromLowerLayers(shared_ptr<ReliableUdpData> p_data);

    /**
     * 返回第dIndex索引对应的字节序列，及其长度dDataLength
     */
    //shared_ptr<BYTE> SendDataByIndex(const DWORD dIndex, DWORD& dDataLength) { return m_storage->GetDataByIndex(dIndex, dDataLength); }

    /**
     * 将接收队列里的数据交付给上层模块，并生成FACK
     */
    //shared_ptr<Force_ACK_Message> TraverseForceACK(DWORD dTermId);
private:

    /**
     * 从数组中按照索引返回某个元素
     */


    /**
     * 根据接收到的FACK设置数据发送队列队首元素的索引，dIndex必须在循环队列的有效范围内
     * 此函数旨在缩小循环队列有效范围的大小
     */
    //bool SetHeadIndex(shared_ptr<const bbr_feedback_msg_t> p_feedback_message);

    /**
     * 在遍历本地接收队列生成ACK时使用，更改数据接收队列的索引
     */


    /**
     * 直接设置队尾元素的索引，dIndex必须不在循环队列的有效范围内或者与前队尾元素索引相等，但是必须在循环队列的可能范围内（由容量决定）
     * 此函数旨在扩大循环队列有效范围的大小，譬如当做数据接收循环队列时接收到对端发来的新数据包
     */
    //bool SetTailIndex(DWORD dIndex);

    /**
     * 按照参数的索引值，将相应的元素放入数组
     */
    //bool SetItemByIndex(DWORD dIndex, shared_ptr<packet_feedback> p_item);

    /**
     * 如果参数在队列范围内（不包括tail序号），返回true，否则返回false
     */



    //void SetElementNotReceiveByIndex(DWORD dIndex) { m_sendpacketsqueue_array[dIndex]->SetNotReceive(); }
private:
    /**
     * 内部工具函数，判断循环队列队首队尾索引的状态，注意此函数并不是线程安全的，只用作内部使用
     */
    QOS_CircularDataQueueIndexStatus QosSend_GetStatusValidQueue();
};

