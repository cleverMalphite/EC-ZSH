//
// Created by Kong on 2023/6/9.
//

#ifndef RBUDP_END2ENDRELIABLETRANSMISSION_H
#define RBUDP_END2ENDRELIABLETRANSMISSION_H


#include "RBUDPApi.h"
#include "QueueMap_H/RecvDataQueueMap.h"
#include "QueueMap_H/SendDataQueueMap.h"
#include "Transfer_H/NAK_Message.h"
using namespace std;

namespace RBUDP {

    class End2EndReliableTransmission {
    private:
        DWORD m_dSelfTermId = 0;	//本机的终端号
        DWORD m_dRemoteTermId = 0;	//对端的终端号
        DWORD m_dFileSize = 0;		//文件总大小
        bool m_b_state = false;			//0表示套接字失效，1表示套接字可用
        bool m_b_write_able = false;		//false表示套接字缓冲区满，不可写；否则表示可写
        bool m_add_able = true;			//true表示上次发送成功，可以向map中添加新的元素，否则表示上次发送不成功，该元素已经添加过，不需要添加
        std::shared_ptr<NAK_Message>	Snd_Last_NAK;	//存储本端作为接收端最近发送的一次NAK
        shared_ptr<RBUDPDataSenderStore> Last_Store;
        //TODO这一块目前采用的是RBUDP，所以不存在接收端不会接收到ACK，只会接收到存有SACK的NAK，如果到时候写UDT这一块要改

        pthread_mutex_t m_cs;
        pthread_mutex_t m_data_tmp_queue_cs;

        /**
        * RBUDP机制里需要使用的结构
        */
        std::shared_ptr<SendDataQueueMap> m_send_queue_map;
        std::shared_ptr<RecvDataQueueMap> m_recv_queue_map;

    public:
        End2EndReliableTransmission(DWORD dseldTermId, DWORD dRemoteTermId, std::string sendCycleFileName);
        ~End2EndReliableTransmission(void);


        /**
         * 供client使用的接口，发送data中[0, nSend)的元素
         * 如果返回true表示用户发送数据成功，否则表示用户发送数据失败
         */
        bool SendUserData(const std::shared_ptr<BYTE> &data, DWORD dLength);

        void HandleIO(std::shared_ptr<BYTE> pData, DWORD nDataLength);	//异步处理网络事件

        bool OnReceiveIndex(DWORD nRecv);		//异步存放已接受到的索引

        /**
         * 发送FACK
         */
        void SendNAK();

        shared_ptr<const NAK_Message> TraverseNAK();

    private:

        std::string GetSendFilepath(std::string filePathName);	//内部函数，用于拼接字符串形成发送文件名
        std::string GetReceiveFilePath(std::string filePathName); ////内部函数，用于拼接字符串形成接收文件名
        /**
         * 供发送字节序列，把data里[0, nLength)的数据发出去
         */
        bool SendByteData(std::shared_ptr<BYTE> data, DWORD nLength) const;

        /**
         * 处理接收到的数据对应的字节序列
        @Param szBytes 指的是接收到的那个UDP包的字节序列
        @Param nRecv 指的是接收到的那个UDP包字节序列中的有效字节长度（单位是字节）
        */
        void OnReceiveData(std::shared_ptr<BYTE> szBytes, int nRecv);

        /**
         * 处理接收到的原始信令对应的字节序列
         @Param szBytes 指的是接收到的那个UDP包的字节序列
         @Param nRecv 指的是接收到的那个UDP包字节序列中的有效字节长度（单位是字节）
         */
        void OnReceiveMessage(std::shared_ptr<BYTE> szBytes, int nRecv);
        /**
         * 表示收到不可靠传输数据
         */
        void OnReceiveWithoutReliableData(std::shared_ptr<BYTE> szBytes, int nRecv);



    public:
        void UploadRecvcQueue();
        /**
         * 处理接收到的信令
         */
        void OnReceiveMessage(std::shared_ptr<NAK_Message> message);

        void OnCheckTimeOut();

    private:
        DWORD m_try_send_number;	//调用SendByteData发送数据的次数
        DWORD m_retry_send_number;	//重传包的个数
        float m_retry_rate = 0.0;			//重传率，也是丢包率
    public:
        float GetRetryRate() {
            return m_retry_rate;
        }
        inline void AddTrySendNumber() {
            /*if (m_try_send_number > 100000)
            {
                ComputeRetryRate();
            }*/
            m_try_send_number++;
        };
        inline void AddRetrySendNUmber(DWORD d_num)
        {
            /*if (m_retry_send_number > 100000)
            {
                ComputeRetryRate();
            }*/
            m_retry_send_number += d_num;
        };
        float ComputeRetryRate();	//计算从上一次调用此方法到现在，本端作为发送端的丢包率，经过考量，本模块不周期性调用此方法，交由CONVERT模块周期性调用
        bool AdjustBandWidth();
    };
}
#endif //RBUDP_END2ENDRELIABLETRANSMISSION_H
