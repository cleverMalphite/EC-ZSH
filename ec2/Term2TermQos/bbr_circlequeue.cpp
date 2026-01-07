#include "bbr_circlequeue.h"
#include "bbr_controller.h"
#include <queue>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
bool CircularDataQueue::AddItem(shared_ptr<packet_feedback> p_item)
{
    //GlobalCritialSection cs(m_cs);
    //m_cs;

    pthread_mutex_lock(&m_cs);
    //先判断队列是否已满
    if (m_queue_size == m_queue_capacity)
    {
        return false;
    }

    //在队尾加入元素，注意不一定是vector的尾部
    m_queue_size++;
    m_queue_array[m_queue_tail] = p_item;
    //在这里更新p_item的内部选项
    //p_item->AddToQueueSuccess(m_queue_tail);
    //存入存储介质
    //m_storage->StorageData(p_item);
    //然后把p_item里关联到数据的智能指针赋值为nullptr，这一步可以节省内存
    //p_item->SetDataNull();
    //
    m_queue_tail++;
    m_queue_tail = m_queue_tail % m_queue_capacity;
    pthread_mutex_unlock(&m_cs);
    return true;
}
bool CircularDataQueue::Qos_DealSingleSendData(const shared_ptr<packet_feedback>& p_item)
{
    //如果接收成功
    //p_remote_to_self->SetReceive();
    //printf("Run SelfToRemoteQosUnit::DealSelfToRemoteQosInfo()6\n");
    const DWORD dIndex = p_item->sequence_number;
    //判断包是否有效
    shared_ptr<packet_feedback> pTemp = m_queue_array[dIndex];	//取出目前队列内的该元素
    if ((p_item->send_ts > pTemp->send_ts) /*&& !pTemp->IsReceive()*/)
    {
        //新来的包时效性较强，那么就把该包替换掉
        m_queue_array[dIndex] = p_item;	//注意这里千万不能把元素里面的数据部分置为nullptr /////////const转换出现问题
        //更改循环队列内容，如果dIndex在原有效队列外，还要把新加入队列的值设置为false，否则设置为true
        if (IsIndexInValid(dIndex))
        {
            //如果在有效队列内，不需要做什么
        }
        else
        {
            //如果不在有效队列内，那么需要扩展队列，并从原tail（包括tail）起，把新增加入队列的元素（此元素除外）的收到标志置为false
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
    return false;
}
shared_ptr<packet_feedback> CircularDataQueue::GetItemByIndex(DWORD dIndex)
{
    //如果Index在循环队列范围内
    if (dIndex >= 0 && dIndex <= m_queue_capacity - 1)
    {
        return m_queue_array.at(dIndex);
    }

    return nullptr;
}
bool CircularDataQueue::SetItemByIndex(DWORD dIndex, shared_ptr<packet_feedback> p_item)
{
    if (dIndex >= 0 && dIndex <= m_queue_capacity - 1)
    {
        m_queue_array[dIndex] = p_item;
        return true;
    }

    return false;
}
bool CircularDataQueue::SetHeadIndex(DWORD dIndex)
{
    if (dIndex == m_queue_head) {
        //意味着队列没有变化
        return true;
    }

    if (dIndex == m_queue_tail)
    {
        //意味着队列大小变为0
        while (m_queue_head != m_queue_tail) {
            m_queue_array[m_queue_head]->payload_size = 0;
            m_queue_head = (m_queue_head + 1) % m_queue_capacity;
            m_queue_size--;
        }
        //m_queue_head = m_queue_tail;
        m_queue_size = 0;
        return true;
    }
    else {
        while (m_queue_head != dIndex) {
            m_queue_array[m_queue_head]->payload_size = 0;
            m_queue_head = (m_queue_head + 1) % m_queue_capacity;
            m_queue_size--;
        }
        return true;
    }
    //先判断dIndex是否在队列里
    //QOS_CircularDataQueueIndexStatus index_status = GetStatusValidQueue();
    //switch (index_status)
    //{
    //case QOS_Queue_Head_Index_Greater_Than_Queue_Tail_Index:
    //{
    //	if (dIndex > m_queue_tail &&  dIndex < m_queue_head) { return false; } //超出有效范围

    //	m_queue_head = dIndex;
    //	if (m_queue_head < m_queue_tail) { m_queue_size = m_queue_tail - m_queue_head; }
    //	else if (m_queue_head > m_queue_tail) { m_queue_size = m_queue_tail + (m_queue_capacity - m_queue_head); } //千万要注意要先减去，避免溢出
    //	else { m_queue_size = 0; }
    //}
    //return true;
    //case QOS_Queue_Head_Index_Less_Than_Queue_Tail_Index:
    //{
    //	if (dIndex < m_queue_head || dIndex > m_queue_tail) { return false; }
    //	m_queue_head = dIndex;
    //	m_queue_size = m_queue_tail - m_queue_head;
    //}
    //return true;
    //case QOS_Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Full:
    //{
    //	m_queue_head = dIndex;
    //	const QOS_CircularDataQueueIndexStatus internal_index_status = GetStatusValidQueue();
    //	if (QOS_Queue_Head_Index_Greater_Than_Queue_Tail_Index == internal_index_status)
    //	{
    //		m_queue_size = m_queue_tail + m_queue_capacity - m_queue_head;
    //	}
    //	else if (QOS_Queue_Head_Index_Less_Than_Queue_Tail_Index == internal_index_status)
    //	{
    //		m_queue_size = m_queue_tail - m_queue_head;
    //	}
    //	else { m_queue_size = 0; }
    //}
    //return true;
    //case QOS_Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Zero:
    //{}
    //return true;
    //case QOS_Wrong_Head_Tail_Index_Status:
    //{

    //}
    //return false;
    //default:
    //	return false;
    //}
}
bool CircularDataQueue::IsIndexInValid(DWORD dIndex)
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

QOS_CircularDataQueueIndexStatus CircularDataQueue::GetStatusValidQueue()
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