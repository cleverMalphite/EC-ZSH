//
// Created by 王炳棋 on 2022/12/28.
//
#ifndef NETCOMBTRANSFER_END2ENDRELIABLETRANSMISSION_H
#define NETCOMBTRANSFER_END2ENDRELIABLETRANSMISSION_H

#include "MRUDPApi.h"
#include "ReliableTransferLog.h"
#include "Transfer_H/ReliableUdpData.h"
#include "Transfer_H/ReliableUdpMessage.h"
#include "CircularQueue_H/SendCircularDataQueue.h"
#include "CircularQueue_H/RecvCircularDataQueue.h"

namespace MRUDP {
    class CycleFile;

    /**
	 * 这个类抽象出了RUDP的socket
	 */
    class End2EndReliableTransmission {
    public:
        End2EndReliableTransmission(DWORD dSelfTermId, DWORD dRemoteTermId, const std::string& SendCycleFileName);

        ~End2EndReliableTransmission();

    private:
        DWORD m_dSelfTermId = 0;    //本机终端
        DWORD m_dRemoteTermId = 0;  //对端终端
        bool m_b_state = false;     //表示套接字状态
        bool m_b_write_able = false;//表示套接字是否可写
        std::shared_ptr<Force_ACK_Message> force_ack_message_we_receive_last_time = nullptr;    //存储本端作为发送端接收到的上一个ACK
        std::shared_ptr<ReliableTransferLog> m_reliable_transfer_log = std::make_shared<ReliableTransferLog>();

        pthread_mutex_t Mutex_;
        /**
		 * RUDP机制里需要使用的结构
		 */
        std::shared_ptr<SendCircularDataQueue> m_send_circular_queue = nullptr;
        std::shared_ptr<RecvCircularDataQueue> m_receive_circular_queue = nullptr;

        DWORD m_try_send_number;      //调用SendByteData发送数据的次数
        DWORD m_retry_send_number;    //重传包的个数
        float m_retry_rate = 0.0;     //重传率，也是丢包率

    public:

        void UpdateRecvQueue();

        void UploadRecvQueue();
        /**
		 * 供client使用的接口，发送data中[0, nSend)的元素
		 * 如果返回true表示用户发送数据成功，否则表示用户发送数据失败
		 */
        bool SendUserData(const std::shared_ptr<BYTE>& data, DWORD dLength);

        //异步处理网络事件
        void HandleIO(const std::shared_ptr<BYTE>& pData, DWORD nDataLength);

        /**
		 * 发送FACK
		 */
        void SendFACK();

        std::shared_ptr<const Force_ACK_Message> TraversForceACK();

    private:
        //内部函数,用于拼接字符串形成发送文件名
        static std::string GetSendFilePath(const std::string& filePathName);

        //内部函数，用于拼接字符串形成接收文件名
        static std::string GetReceiveFilePath(const std::string& filePathName);

        /**
		 * 供发送字节序列，把data里[0, nLength)的数据发出去
		 */
        bool SendByteData(const std::shared_ptr<BYTE>& data, DWORD nLength) const;

        /**
		 * 处理接收到的数据对应的字节序列
		 @Param szBytes 指的是接收到的那个UDP包的字节序列
		 @Param nRecv 指的是接收到的那个UDP包字节序列中的有效字节长度（单位是字节）
		 */
        void OnReceiveData(const std::shared_ptr<BYTE>& szBytes, int nRecv);

        /**
         * 处理接收到的原始信令对应的字节序列
         @Param szBytes 指的是接收到的那个UDP包的字节序列
         @Param nRecv 指的是接收到的那个UDP包字节序列中的有效字节长度（单位是字节）
         */
        void OnReceiveMessage(const std::shared_ptr<BYTE>& szBytes, int nRecv);

    public:
        /**
		 * 处理接收到的信令
		 */
        void OnReceiveMessage(const std::shared_ptr<Force_ACK_Message>& pMessage);

    private:
        /**
		 * 表示收到不可靠传输数据
		 */
        void OnReceiveWithoutReliableData(std::shared_ptr<BYTE> szBytes, int nRecv) const;

    public:
        float GetRetryRate() const {
            return m_retry_rate;
        }

        inline void AddTrySendNumber() {
            if (m_try_send_number > 100000) {
                ComputeRetryRate();
            }
            m_try_send_number++;
        };

        inline void AddRetrySendNumber(DWORD d_num) {
            if (m_retry_send_number > 100000) {
                ComputeRetryRate();
            }
            m_retry_send_number += d_num;
        };

        //计算从上一次调用此方法到现在，
        //本端作为发送端的丢包率，经过考量，本模块不周期性调用此方法，交由CONVERT模块周期性调用
        float ComputeRetryRate();

        bool AdjustBandWidth();
    };
}
#endif //NETCOMBTRANSFER_END2ENDRELIABLETRANSMISSION_H
