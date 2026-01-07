#pragma once

#include <queue>
#include <memory>
#include <set>
#include "TermToTermQosApi.h"
//#include "QosStructInterfaceInfo.h"
#include "../SpeedControl/QosStructInterfaceInfo.h"
#include "Remote2SelfPacketQosFeedbackPacket.h"
#include "bbr_controller.h"
#include "bbr_received_circle_packet.h"
using std::shared_ptr;
using std::queue;

namespace Term2TermQos
{
    /**接收端
     * 这个类用于处理的输入为：
     * 输入1：对端发送给本端的数据包被本端NetCombTransfer模块偷窥得到的包序号、时间戳等信息。
     *
     * 输出：
     * 向对端发送的反馈包
     */
    class Remote2SelfQosUnit
    {
    public:
        Remote2SelfQosUnit(DWORD d_self_tid, DWORD d_remote_tid);
        virtual ~Remote2SelfQosUnit();

        //接收数据输入1
        void RecvRemoteToSelfQosInfo(shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_qos_info);
        //会被周期性调用的函数，在此函数里调用DealRemoteToSelfQosInfo
        void DoCycleTime();


    public:
        //BBR接收端相关函数
        size_t count ;
        int packetcount = 0;
        bool is_recv_start = false;
        int64_t send_time;
        size_t accumulated_count = 0;
        int first_packet;
        int first_recv_packet_index;
        int packetseqroundtrip ;
        //shared_ptr< RemoteToSelfQosInfo> p_remote_to_self_info_last_time = make_shared<RemoteToSelfQosInfo>();
        int  last_packet;
        int64_t previousseq = 0;
        int64_t previous_in_queue;
        int64_t previous_feedback_sendtimestamp = 0;
        ReceivdCircularDataQueue  *storgepacketsnumber;
        //set<int64_t> storgepacketsnumber;
        int64_t last_packet_sendtime;
        int64_t last_recv_time;
        int64_t last_recv_time_test = GET_SYS_TIME();//////仅测试用
        int64_t losstime = -1;
        int     active_wnd_size;
        uint32_t acked_bitrate;
        cc_loss_statistics_t loss_stat;
        bin_stream_t		 strm;
        skiplist_t*          cache;
        int64_t              base_seq;            /*窗口反馈起始ID*/
        int32_t	             big_endian = -1;
        int32_t		         bin_stream_init(void* ptr);
        void                 loss_statistics_init(cc_loss_statistics_t* loss_stat);
        void                 loss_statistics_incoming(cc_loss_statistics_t* loss_stat, uint16_t seq);
        int				     loss_statistics_calculate(cc_loss_statistics_t* loss_stat, int64_t now_ts, uint8_t* fraction_loss, int* loss_num, int* num);


    private:
        const DWORD m_d_self_tid;	//本端的终端号
        const DWORD m_d_remote_tid;	//对端的终端号
        queue<shared_ptr<const RemoteToSelfQosInfo> > m_remote_to_self_qos_info_queue;	//数据输入1的队列
        pthread_mutex_t m_cs;
        //处理数据输入1
        void DealRemoteToSelfQosInfo();
    };
}
