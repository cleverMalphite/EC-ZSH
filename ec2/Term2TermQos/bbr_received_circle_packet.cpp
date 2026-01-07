#include "bbr_received_circle_packet.h"
#include <queue>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

shared_ptr<RemoteToSelfQosInfo> ReceivdCircularDataQueue::Qos_GetItemByIndex(DWORD dIndex)
{
    //如果Index在循环队列范围内
    if (dIndex >= 0 && dIndex <= m_queue_capacity - 1)
    {
        return m_queue_array.at(dIndex);
    }

    return nullptr;
}
bool ReceivdCircularDataQueue::Qos_DealSingleReceiveData( shared_ptr<RemoteToSelfQosInfo> p_remote_to_self)
{
    //如果接收成功
    //p_remote_to_self->SetReceive();
    const DWORD dIndex = p_remote_to_self->m_d_index;
    //判断包是否有效
    shared_ptr<RemoteToSelfQosInfo> pTemp = m_queue_array[dIndex];	//取出目前队列内的该元素
    if ((p_remote_to_self->m_d_timestamp > pTemp->m_d_timestamp) /*&& !pTemp->IsReceive()*/)
    {
        //新来的包时效性较强，那么就把该包替换掉
        m_queue_array[dIndex] = p_remote_to_self;	//注意这里千万不能把元素里面的数据部分置为nullptr /////////const转换出现问题
        //更改循环队列内容，如果dIndex在原有效队列外，还要把新加入队列的值设置为false，否则设置为true
        if (Qos_IsIndexInValid(dIndex))
        {
            //如果在有效队列内，不需要做什么
        }
        else
        {
            //如果不在有效队列内，那么需要扩展队列，并从原tail（包括tail）起，把新增加入队列的元素（此元素除外）的收到标志置为false
            DWORD tail_tmp = m_queue_tail;
            DWORD distance = 0;
            while (tail_tmp != dIndex)
            {
                //SetElementNotReceiveByIndex(m_queue_tail);
                //std::cout << "lost packets number:" << m_queue_tail<<std::endl;
                tail_tmp = (tail_tmp + 1) % m_queue_capacity;
                distance++;
            }
            if (2*distance > m_queue_capacity) { distance = 0.5*m_queue_capacity; return false; }
            while (m_queue_tail != dIndex)
            {
                //SetElementNotReceiveByIndex(m_queue_tail);
                //std::cout << "lost packets number:" << m_queue_tail<<std::endl;
                m_queue_tail = (m_queue_tail + 1) % m_queue_capacity;
                m_queue_size++;
            }
            m_queue_size++;
            //m_queue_array[dIndex]->SetReceive();
            m_queue_tail = (dIndex + 1) % m_queue_capacity;
            //重新标注索引
        }
        return true;
    }
}
//bool ReceivdCircularDataQueue::SetItemByIndex(DWORD dIndex, shared_ptr<packet_feedback> p_item)
//{
//	if (dIndex >= 0 && dIndex <= m_queue_capacity - 1)
//	{
//		m_queue_array[dIndex] = p_item;
//		return true;
//	}
//
//	return false;
//}
//bool ReceivdCircularDataQueue::SetHeadIndex(shared_ptr<const bbr_feedback_msg_t> p_feedback_message)
//{
//	if (0 == m_queue_size)
//	{
//		return true;
//	}
//	/**
//	 * 首先要确保其确认的尾包在本数据接收队列的有效区间内，分两步，一步看尾包时间戳，尾包时间戳如果不对那就肯定不对
//	 */
//	const DWORD d_tail_packet_timestamp = p_feedback_message->last_num_send_time;	//取得尾包时间戳
//	const DWORD d_tail_packet_index = p_feedback_message->last_num;		    //取得尾包序号
//	//取出数据接收队列的相应包
//	shared_ptr<packet_feedback> p_item = m_queue_array[p_feedback_message->last_num];
//	if (p_item->send_ts != d_tail_packet_timestamp || !IsIndexInValid(d_tail_packet_index))
//	{
//		//如果1. 已经收到数据接收队列相应包; 2.或者FACK所携带的尾包时间戳与相应包时间戳不等；
//		// 3.尾包序号不在数据接收队列有效部分序列范围。         =>那么直接返回
//		return true;
//	}
//	else
//	{
//		//将队列首元素置为尾包的下一个包
//		m_queue_head = (d_tail_packet_index + 1) % m_queue_capacity;
//		//下面开始快速计算新的数据接收队列有效序列范围的大小
//		const QOS_CircularDataQueueIndexStatus internal_index_status = GetStatusValidQueue();
//		if (QOS_Queue_Head_Index_Greater_Than_Queue_Tail_Index == internal_index_status)
//		{
//			m_queue_size = m_queue_tail + m_queue_capacity - m_queue_head;
//		}
//		else if (QOS_Queue_Head_Index_Less_Than_Queue_Tail_Index == internal_index_status)
//		{
//			m_queue_size = m_queue_tail - m_queue_head;
//		}
//		else { m_queue_size = 0; }
//	}
//	return true;
//}
bool ReceivdCircularDataQueue::Qos_SetHeadIndex(DWORD dIndex)
{
    if (dIndex == m_queue_head) {
        //意味着队列没有变化
        return true;
    }

    if (dIndex == m_queue_tail)
    {
        //意味着队列大小变为0
        m_queue_head = m_queue_tail;
        m_queue_size = 0;
        return true;
    }

    //先判断dIndex是否在队列里
    QOS_CircularDataQueueIndexStatus index_status = Qos_GetStatusValidQueue();
    switch (index_status)
    {
        case QOS_Queue_Head_Index_Greater_Than_Queue_Tail_Index:
        {
            if (dIndex > m_queue_tail &&  dIndex < m_queue_head) { return false; } //超出有效范围

            m_queue_head = dIndex;
            if (m_queue_head < m_queue_tail) { m_queue_size = m_queue_tail - m_queue_head; }
            else if (m_queue_head > m_queue_tail) { m_queue_size = m_queue_tail + (m_queue_capacity - m_queue_head); } //千万要注意要先减去，避免溢出
            else { m_queue_size = 0; }
        }
            return true;
        case QOS_Queue_Head_Index_Less_Than_Queue_Tail_Index:
        {
            if (dIndex < m_queue_head || dIndex > m_queue_tail) { return false; }
            m_queue_head = dIndex;
            m_queue_size = m_queue_tail - m_queue_head;
        }
            return true;
        case QOS_Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Full:
        {
            m_queue_head = dIndex;
            const QOS_CircularDataQueueIndexStatus internal_index_status = Qos_GetStatusValidQueue();
            if (QOS_Queue_Head_Index_Greater_Than_Queue_Tail_Index == internal_index_status)
            {
                m_queue_size = m_queue_tail + m_queue_capacity - m_queue_head;
            }
            else if (QOS_Queue_Head_Index_Less_Than_Queue_Tail_Index == internal_index_status)
            {
                m_queue_size = m_queue_tail - m_queue_head;
            }
            else { m_queue_size = 0; }
        }
            return true;
        case QOS_Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Zero:
        {}
            return true;
        case QOS_Wrong_Head_Tail_Index_Status:
        {

        }
            return false;
        default:
            return false;
    }
}
DWORD ReceivdCircularDataQueue::QOS_GetDistance_dIndex_to_tail(DWORD dIndex) {
    DWORD tail_tmp = m_queue_tail;
    DWORD distance = 0;
    while (tail_tmp != dIndex)
    {
        //SetElementNotReceiveByIndex(m_queue_tail);
        //std::cout << "lost packets number:" << m_queue_tail<<std::endl;
        tail_tmp = (tail_tmp + 1) % m_queue_capacity;
        distance++;
    }
    return distance;
}
bool ReceivdCircularDataQueue::Qos_IsIndexInValid(DWORD dIndex)
{
    if (0 == m_queue_size)
    {
        return false;
    }
    if (m_queue_head > m_queue_tail)
    {
        if (dIndex >= m_queue_tail && dIndex < m_queue_head) { return false; }
        else { return true; }
    }

    if (m_queue_head < m_queue_tail)
    {
        if (dIndex >= m_queue_head && dIndex < m_queue_tail) { return true; }
        else { return false; }
    }

    if (m_queue_head == m_queue_tail)
    {
        //特殊情况，有效队首、队尾元素索引相等
        if (m_queue_capacity == m_queue_size)
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

QOS_CircularDataQueueIndexStatus ReceivdCircularDataQueue::Qos_GetStatusValidQueue()
{
    if (m_queue_head > m_queue_tail)
    {
        return QOS_Queue_Head_Index_Greater_Than_Queue_Tail_Index;
    }

    if (m_queue_head < m_queue_tail)
    {
        return QOS_Queue_Head_Index_Less_Than_Queue_Tail_Index;
    }

    if (m_queue_head == m_queue_tail)
    {
        if (m_queue_capacity == m_queue_size)
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