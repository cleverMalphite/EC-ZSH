#include "bbr_sendhistory_circle_queue.h"

#include <queue>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
shared_ptr<bbr_packet_point_t> SendCircularDataQueue::QosSend_GetItemByIndex(DWORD dIndex)
{
    //如果Index在循环队列范围内
    if (dIndex >= 0 && dIndex <= m_sendpacketsqueue_capacity - 1)
    {
        return m_sendpacketsqueue_array.at(dIndex);

    }

    return nullptr;
}
DWORD SendCircularDataQueue::QosSend_SetPosition(DWORD pos) {
    while (m_sendpacketsqueue_tail != pos)
    {
        //SetElementNotReceiveByIndex(m_queue_tail);
        //std::cout << "lost packets number:" << m_queue_tail<<std::endl;
        m_sendpacketsqueue_tail = (m_sendpacketsqueue_tail + 1) % m_sendpacketsqueue_capacity;
        m_sendpacketsqueue_tail++;
    }
    m_sendpacketsqueue_tail_tmp = m_sendpacketsqueue_tail;
    m_sendpacketsqueue_size++;
    //m_queue_array[dIndex]->SetReceive();
    m_sendpacketsqueue_tail = (pos + 1) % m_sendpacketsqueue_capacity;
    return m_sendpacketsqueue_tail_tmp;
}
bool SendCircularDataQueue::QosSend_AddItem(shared_ptr<bbr_packet_point_t> p_item)
{
    pthread_mutex_lock(&m_cs);
    //先判断队列是否已满
    if (m_sendpacketsqueue_size == m_sendpacketsqueue_capacity)
    {
        pthread_mutex_unlock(&m_cs);
        return false;
    }

    //在队尾加入元素，注意不一定是vector的尾部
    m_sendpacketsqueue_size++;
    m_sendpacketsqueue_array[m_sendpacketsqueue_tail] = p_item;
    //在这里更新p_item的内部选项
    //p_item->AddToQueueSuccess(m_queue_tail);
    //存入存储介质
    //m_storage->StorageData(p_item);
    //然后把p_item里关联到数据的智能指针赋值为nullptr，这一步可以节省内存
    //p_item->SetDataNull();
    //
    m_sendpacketsqueue_tail++;
    m_sendpacketsqueue_tail = m_sendpacketsqueue_tail % m_sendpacketsqueue_capacity;
    pthread_mutex_unlock(&m_cs);
    return true;
}
bool SendCircularDataQueue::QosSend_SetHeadIndexStepByStep(DWORD dHeaderIndex)
{
    //DealReceiveData();	//先处理暂存数据接收队列中的数据
    //queue<shared_ptr<ReliableUdpData> > data_call_back_upper;	//反馈给上层模块的本模块接收数据队列
    //shared_ptr<Force_ACK_Message> p_fack = nullptr;		//最终返回的fack
    {
        //局部同步
        MutexLockGuard gGuard(m_cs);
        //判断当下数据接收队列中有效元素数目是否为空
        if (0 == m_sendpacketsqueue_size)
        {
            //m_force_ack_message_we_send_last_time->SetTimeStamp(GetTickCount());	//更新FACK的时间
            //p_fack = m_force_ack_message_we_send_last_time;
            //return p_fack;
            return false;
        }
        const DWORD d_old_queue_size = m_sendpacketsqueue_size;	//即将可能成为历史的有效队列大小
        //从head开始遍历
        DWORD dTempIndex = m_sendpacketsqueue_head;
        DWORD d_number_of_packet_have_vertify = 0;	//已经被检查的包数目
        while (dTempIndex!= dHeaderIndex/*d_number_of_packet_have_vertify < d_old_queue_size*/)
        {
            shared_ptr<bbr_packet_point_t> p_data = m_sendpacketsqueue_array[dTempIndex];
            if (!p_data->ignore==1)
            {

                //如果该元素没有收到，那么直接跳出循环
                break;
            }
            else
            {
                //p_data->send_time = 0;
                p_data->size = 0;
                p_data->total_data_sent = 0;
                p_data->total_data_acked_at_the_last_recv_fbpacket= 0;
                p_data->total_data_sent_at_last_acked_packet = 0;
                p_data->last_acked_packet_ack_time = -1;
                p_data->last_acked_packet_sent_time = -1;
                p_data->is_app_limited = 0;
                //p_data->ignore == 1;
                dTempIndex = (dTempIndex + 1) % m_sendpacketsqueue_capacity;
                m_sendpacketsqueue_size--;
            }
        }

        if (d_old_queue_size == m_sendpacketsqueue_size)
        {
            return false;
        }

        //先更新有效队列
        m_sendpacketsqueue_head = dTempIndex;
        dTempIndex = (dTempIndex + m_sendpacketsqueue_capacity - 1) % m_sendpacketsqueue_capacity;	//获得最后一个确认的数据包，注意不要直接减，防止出现负值
        //一旦跳出，就可以生成FACK
        //p_fack = make_shared<Force_ACK_Message>(GetTickCount(), dTempIndex, m_queue_array[dTempIndex]->GetTimeStamp());
        //m_force_ack_message_we_send_last_time = p_fack;
    }
    //退出临界区后再反馈给上层模块相应数据
    //shared_ptr<ReliableUdpData> p_data_temp = nullptr;
    //while (0 < data_call_back_upper.size())
    //{
    //	p_data_temp = data_call_back_upper.front();
    //	CallBackDataFunc(dTermId, p_data_temp->GetData(), p_data_temp->GetDataLength());
    //	data_call_back_upper.pop();
    //}
    return true;
}

//bool SendCircularDataQueue::QosSend_SetHeadIndex(DWORD dIndex)
//{
//	if (dIndex == m_sendpacketsqueue_head) {
//		//意味着队列没有变化
//		return true;
//	}
//
//	if (dIndex == m_sendpacketsqueue_tail)
//	{
//		//意味着队列大小变为0
//		m_sendpacketsqueue_head = m_sendpacketsqueue_tail;
//		m_sendpacketsqueue_size = 0;
//		return true;
//	}
//
//	//先判断dIndex是否在队列里
//	QOS_CircularDataQueueIndexStatus index_status = Qos_GetStatusValidQueue();
//	switch (index_status)
//	{
//	case QOS_Queue_Head_Index_Greater_Than_Queue_Tail_Index:
//	{
//		if (dIndex > m_sendpacketsqueue_tail &&  dIndex < m_sendpacketsqueue_head) { return false; } //超出有效范围
//
//		m_sendpacketsqueue_head = dIndex;
//		if (m_sendpacketsqueue_head < m_sendpacketsqueue_tail) { m_sendpacketsqueue_size = m_sendpacketsqueue_tail - m_sendpacketsqueue_head; }
//		else if (m_sendpacketsqueue_head > m_sendpacketsqueue_tail) { m_sendpacketsqueue_size = m_sendpacketsqueue_tail + (m_sendpacketsqueue_capacity - m_sendpacketsqueue_head); } //千万要注意要先减去，避免溢出
//		else { m_sendpacketsqueue_size = 0; }
//	}
//	return true;
//	case QOS_Queue_Head_Index_Less_Than_Queue_Tail_Index:
//	{
//		if (dIndex < m_sendpacketsqueue_head || dIndex > m_sendpacketsqueue_tail) { return false; }
//		m_sendpacketsqueue_head = dIndex;
//		m_sendpacketsqueue_size = m_sendpacketsqueue_tail - m_sendpacketsqueue_head;
//	}
//	return true;
//	case QOS_Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Full:
//	{
//		m_sendpacketsqueue_head = dIndex;
//		const QOS_CircularDataQueueIndexStatus internal_index_status = Qos_GetStatusValidQueue();
//		if (QOS_Queue_Head_Index_Greater_Than_Queue_Tail_Index == internal_index_status)
//		{
//			m_sendpacketsqueue_size = m_sendpacketsqueue_tail + m_sendpacketsqueue_capacity - m_sendpacketsqueue_head;
//		}
//		else if (QOS_Queue_Head_Index_Less_Than_Queue_Tail_Index == internal_index_status)
//		{
//			m_sendpacketsqueue_size = m_sendpacketsqueue_tail - m_sendpacketsqueue_head;
//		}
//		else { m_sendpacketsqueue_size = 0; }
//	}
//	return true;
//	case QOS_Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Zero:
//	{}
//	return true;
//	case QOS_Wrong_Head_Tail_Index_Status:
//	{
//
//	}
//	return false;
//	default:
//		return false;
//	}
//}
bool SendCircularDataQueue::QosSend_IsIndexInValid(DWORD dIndex)
{
    if (0 == m_sendpacketsqueue_size)
    {
        return false;
    }
    if (m_sendpacketsqueue_head > m_sendpacketsqueue_tail)
    {
        if (dIndex >= m_sendpacketsqueue_tail && dIndex < m_sendpacketsqueue_head) { return false; }
        else { return true; }
    }

    if (m_sendpacketsqueue_head < m_sendpacketsqueue_tail)
    {
        if (dIndex >= m_sendpacketsqueue_head && dIndex < m_sendpacketsqueue_tail) { return true; }
        else { return false; }
    }

    if (m_sendpacketsqueue_head == m_sendpacketsqueue_tail)
    {
        //特殊情况，有效队首、队尾元素索引相等
        if (m_sendpacketsqueue_capacity == m_sendpacketsqueue_size)
        {
            return true;	//队列满为在有效范围内
        }
        else
        {
            return false;  //队列空则不在有效范围内
        }
    }

    return true;
}

QOS_CircularDataQueueIndexStatus SendCircularDataQueue::QosSend_GetStatusValidQueue()
{
    if (m_sendpacketsqueue_head > m_sendpacketsqueue_tail)
    {
        return QOS_Queue_Head_Index_Greater_Than_Queue_Tail_Index;
    }

    if (m_sendpacketsqueue_head < m_sendpacketsqueue_tail)
    {
        return QOS_Queue_Head_Index_Less_Than_Queue_Tail_Index;
    }

    if (m_sendpacketsqueue_head == m_sendpacketsqueue_tail)
    {
        if (m_sendpacketsqueue_capacity == m_sendpacketsqueue_size)
        {
            return QOS_Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Full;
        }
        else
        {
            return QOS_Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Zero;
        }
    }

    return QOS_Wrong_Head_Tail_Index_Status;
}