//
// Created by Kong on 2023/6/12.
//

#ifndef RBUDP_RBUDPTEMPDATAQUEUE_H
#define RBUDP_RBUDPTEMPDATAQUEUE_H

#endif //RBUDP_RBUDPTEMPDATAQUEUE_H
#include "RBUDPTempData.h"

/**
 * 此类为按照STL里的队列来拓展得到的线程安全队列
 */
namespace RBUDP
{
    class RBUDPTempDataQueue
    {
    public:
        RBUDPTempDataQueue()
        {
            m_dQueueLength = 0;
            pthread_mutex_init(&m_cs, nullptr);
        }

        virtual ~RBUDPTempDataQueue()
        {
            m_dQueueLength = 0;
            pthread_mutex_destroy(&m_cs);
        }

        bool enqueue(std::shared_ptr<RBUDPTempData> item)
        {
            pthread_mutex_lock(&m_cs);
            m_queue.push(item);
            m_dQueueLength++;
            pthread_mutex_unlock(&m_cs);
            return true;
        }

        //取队首数据，但不删除
        inline std::shared_ptr<RBUDPTempData> dequeue()
        {
            std::shared_ptr<RBUDPTempData> item = nullptr;
            pthread_mutex_lock(&m_cs);
            if (!m_queue.empty()) {
                item = m_queue.front();
            }
            pthread_mutex_unlock(&m_cs);
            return item;
        }

        //删除队首数据
        inline bool del_data()
        {
            pthread_mutex_lock(&m_cs);
            if (m_queue.empty()) {
                pthread_mutex_unlock(&m_cs);
                return false;
            }
            m_queue.pop();
            m_dQueueLength--;
            pthread_mutex_unlock(&m_cs);
            return true;
        }

        virtual DWORD GetQueueLength()
        {
            DWORD dTemp = 0;
            pthread_mutex_lock(&m_cs);
            dTemp = m_dQueueLength;
            pthread_mutex_unlock(&m_cs);
            return dTemp;
        }

        inline bool IsEmpty()
        {
            bool bIsEmpty = false;
            pthread_mutex_lock(&m_cs);
            bIsEmpty = m_queue.empty();
            pthread_mutex_unlock(&m_cs);
            return bIsEmpty;
        }

    private:
        std::queue<std::shared_ptr<RBUDPTempData>> m_queue;
        DWORD m_dQueueLength;	//队列的长度
        pthread_mutex_t m_cs;	//队列内部防止线程同步问题的临界区对象
    };
}

