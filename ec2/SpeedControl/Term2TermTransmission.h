//
// Created by 王炳棋 on 2022/12/8.
//
#include "DataWithPacketInfo.h"
#include "QosStructInterfaceInfo.h"
#include "IniHandleApi.h"
#include <queue>
#include <utility>

#ifndef NETCOMBTRANSFER_TERM2TERMTRANSMISSION_H
#define NETCOMBTRANSFER_TERM2TERMTRANSMISSION_H

namespace SpeedControl {
    int64_t su_get_sys_time_tmp();

    typedef struct DataSpeedControlSend {
        std::shared_ptr<BYTE> pData;
        DWORD dataLength;
        bool m_isForceSleep;
        bool m_isRBUDP;

        DataSpeedControlSend(std::shared_ptr<BYTE> data, DWORD length, bool isForceSleep, bool isRBUDP) :
                pData(std::move(data)), dataLength(length), m_isForceSleep(isForceSleep), m_isRBUDP(isRBUDP) {
            /**/
        }
    } DataSpeedControlSend;

    /*端到端的通信实体*/
    class Term2TermTransmission {
    public:
        Term2TermTransmission(DWORD d_remote_tid, DWORD buffer_size = 100000) :
                m_dest_tid(d_remote_tid), m_d_speed(10), m_buffer_max_size(buffer_size) {
            pthread_mutex_init(&Mutex_, nullptr);
            pthread_mutex_init(&TempSpeedMutex_, nullptr);
            gettimeofday(&m_d_big_cycle_begin_time, nullptr);
            gettimeofday(&m_d_small_cycle_begin_time, nullptr);
            DoChangeExpectSpeed(EXPECT_SPEED);
        }

        ~Term2TermTransmission() {
            pthread_mutex_destroy(&Mutex_);
            pthread_mutex_destroy(&TempSpeedMutex_);
        }

    public:
        /*按照大小循环交替发送数据*/
        bool DoSendData();

        bool PushSendData(const std::shared_ptr<BYTE>& data, DWORD length, bool isSleep, bool isRBUDP);

        bool PushSendDataPri(const std::shared_ptr<BYTE>& data, DWORD length, bool isSleep, bool isRBUDP);

        bool DoSendDataWithoutSpeedControl(const std::shared_ptr<BYTE>& data, DWORD length, bool isRBUDP);

        void RecvTermSpeedInfo(const std::shared_ptr<TermSpeedInfo>& t2t_speed);

        /*处理QoS传来的速率控制信息*/

        bool DealTermSpeedInfo(const std::shared_ptr<TermSpeedInfo>& term_speed_info);

        bool SendTIDCommandWithoutSpeedControl(const std::shared_ptr<BYTE>& data, DWORD length, bool isRBUDP);

        DWORD GetDestTID() {
            return m_dest_tid;
        }

    private:
        pthread_mutex_t Mutex_;
        pthread_mutex_t TempSpeedMutex_;
        const DWORD m_dest_tid; //远端终端号
        DWORD m_d_packet_index = 0; //端到端发包序号
        bool m_speed_change = false;
        const DWORD MIN_DATA_LENGTH_FEEDBACK_FOR_QOS = 1300;
        std::deque<std::shared_ptr<DataSpeedControlSend>> data_send_buffer; //存储将要发送到数据
        DWORD m_buffer_max_size = 0;
        /*此处win版代码为size_t,Linux下的size_t和win的字节数不一样*/
        DWORD m_d_speed = 1000; //预期发送速率
        DWORD m_d_small_cycle = 0;  //预期小循环发包
        DWORD m_d_big_cycle = 0;    //预期大循环发包
        DWORD m_d_expect_time = 0;  //预期大循环所需时间
        const DWORD m_d_expect_time_per_big_cycle = 100;   //速率调整周期
        const DWORD m_d_single_packet_length =
                GetIntegerKeyIni("SpeedControl", "SINGLE_PACKET_LENGTH", 1400);        //目前的包长
        //const DWORD m_d_single_packet_length = 1400;        //目前的包长
        const DWORD m_d_max_small_cycle_of_big_cycle =
                GetIntegerKeyIni("SpeedControl", "Max_Small_Cycle_Of_Big_Cycle", 3);    //小循环最大值不超过大循环的1/n
        //const DWORD m_d_max_small_cycle_of_big_cycle = 3;//小循环最大值不超过大循环的1/n
        const static DWORD MIN_PACKET_LENGTH_IGNORE = 100;
        const static DWORD MAX_PACKET_CAN_DEAL = 100000;
        const static DWORD SPEED_MBPS = 1000000;
        const float EXPECT_SPEED = GetRealValueKeyIni("SpeedControl", "ExpectSpeed", 500) * SPEED_MBPS;
        //const float EXPECT_SPEED = 500 * SPEED_MBPS;
        const DWORD CHECK_FREQUENCY = GetIntegerKeyIni("SpeedControl", "CheckFrequency", 1000);
        //const DWORD CHECK_FREQUENCY = 1000;

        //根据预期发送速率更新实例成员变量的值;
        //Mark 这里选择使用float，但是在实际用法中全部传递的是DWORD，有啥意义呢？
    public:
        void DoChangeExpectSpeed(double d_speed);

    private:
        DWORD m_d_current_small_cycle = 0;  //小循环当前发包数
        DWORD m_d_current_big_cycle = 0;    //大循环当前发包数
        //大循环开始时间戳
        /*DWORD m_d_big_cycle_begin_time;*/
        timeval m_d_big_cycle_begin_time;
        timeval m_d_small_cycle_begin_time;
        DWORD m_d_small_cycle_sleep_time = 0;    //小循环累计休眠时间

        bool IsForceTcp = GetBoolValueKeyIni("Main", "IsForceTcp", false);
        /*bool IsForceTcp = false;*/

        //实际数据发送
        bool DoCycleSend(const std::shared_ptr<DataWithPacketInfo>& pPacket);
    };
}


#endif //NETCOMBTRANSFER_TERM2TERMTRANSMISSION_H
