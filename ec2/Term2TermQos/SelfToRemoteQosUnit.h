#pragma once

#include "../mGlobalDef.h"
#include <queue>
#include <deque>
#include <memory>
#include "TermToTermQosApi.h"
#include "QosStructInterfaceInfo.h"
#include "Remote2SelfPacketQosFeedbackPacket.h"
#include "bbr_controller.h"
using std::shared_ptr;
using std::queue;
//#include "bocd.h"
//using namespace bocd;
namespace Term2TermQos
{
    /**发送端
     * 这个类用于存储并周期性处理：
     * 1. 本端发送给对端的数据包被NetCombTransfer偷窥后反馈给本模块的包序号和时间戳等信息，以下简称数据输入1。
     * 2. 对端发送给本端的反馈包，以下简称数据输入2。
     *
     * 处理结果：反馈给速率控制模块的一个带宽估计值。
     */
    class SelfToRemoteQosUnit
    {
    public:
        SelfToRemoteQosUnit(DWORD d_self_tid, DWORD d_remote_tid);
        virtual ~SelfToRemoteQosUnit();

        //接收数据输入1
        void RecvSelfToRemoteQosInfo(const shared_ptr<const SelfToRemoteQosInfo>& p_self_to_remote_qos_info);
        //接收数据输入2
        void RecvRemote2SelfPacketQosFeedbackPacket(shared_ptr<const Remote2SelfPacketQosFeedbackPacket> p_feedback_packet);
        //会被周期性调用的函数，在此函数里调用DealSelfToRemoteQosInfo和DealRemoteToSelfFeedbackPacket
        void DoCycleTime();

    public:
        //BBR发送端相关函数

        int                         payload_size;
        bbr_controller_t*			bbr;
        uint8_t					    m_fraction_loss;
        int32_t	                    big_endian = -1;
        uint32_t					last_bitrate_bps;
        uint8_t						last_fraction_loss;
        uint32_t                    last_rtt;
        uint32_t					max_bitrate;
        uint32_t					min_bitrate;
        uint32_t					target_bitrate;
        double						encoding_rate_ratio;
        int64_t						notify_ts;
        bin_stream_t                stream;
        bbr_network_ctrl_update_t	info;

        int64_t                     datatotlesendlastack = 0;
        int64_t                    datatotlesendlastack_acktime;
        int                    is_bandwidth_full;
        bool                   is_recv_block = false;
        int                    recv_block_stop_sampling_num = 0;
        int64_t                recv_block_stop_time = 0;
        //shared_ptr <BOCD> bcd=make_shared<BOCD>(250, 20, 2, 1.15, 0.01);
        //初始化
        //int32_t                     bin_stream_init(void* ptr);
        void                        bbr_feedback_adapter_init(bbr_fb_adapter_t* adapter);
        void                        rate_stat_init(rate_stat_t* rate, int wnd_size, float scale);
        sender_history_t*           sender_history_create(uint32_t limited_ms);
        bbr_controller_t*	        bbr_create(bbr_target_rate_constraint_t* co, int32_t starting_bandwidth);

        //销毁
        void bbr_feedback_adapter_destroy(bbr_fb_adapter_t* adapter);
        void sender_history_destroy(sender_history_t* hist);
        void rate_stat_destroy(rate_stat_t* rate);
        void bbr_destroy(bbr_controller_t* bbr);
        void sampler_destroy(bbr_bandwidth_sampler_t* sampler);

        //sampler处理
        bbr_bandwidth_sampler_t*    sampler_create();
        void                        sampler_reset(bbr_bandwidth_sampler_t* sampler);
        void                        sender_history_add(sender_history_t* hist, packet_feedback* packet);
        void                        sampler_on_packet_sent(bbr_bandwidth_sampler_t* sampler, int64_t sent_time, int64_t packet_number, size_t data_size, size_t data_in_flight);
        void                        sampler_add_point(bbr_bandwidth_sampler_t* sampler, int64_t sent_time, int64_t number, size_t data_size);
        void                        sampler_resize_points(bbr_bandwidth_sampler_t* sampler, int size);
        inline void                 sampler_to_point(bbr_bandwidth_sampler_t* sampler, shared_ptr<bbr_packet_point_t> point);
        size_t                      sampler_total_data_acked(bbr_bandwidth_sampler_t* sampler);
        void                        sampler_remove_point(bbr_bandwidth_sampler_t* sampler, int64_t number, int packetseqroundtrip);
        bbr_bandwidth_sample_t      sampler_on_packet_acked(bbr_bandwidth_sampler_t* sampler, int64_t ack_time, int64_t fb_send_time, shared_ptr<bbr_packet_point_t> point, uint32_t acked_bitrate);
        bbr_bandwidth_sample_t      sampler_on_packet_acked_inner(bbr_bandwidth_sampler_t* sampler, int64_t ack_time, int64_t fb_send_time, shared_ptr<bbr_packet_point_t> point, uint32_t acked_bitrate);
        void                        sampler_on_app_limited(bbr_bandwidth_sampler_t* sampler);
        void                        sampler_remove_old(bbr_bandwidth_sampler_t* sampler, int64_t least_unacked, int packetseqroundtrip);

        //带宽估计
        void   bbr_on_send_packet(bbr_controller_t* bbr, bbr_packet_info_t* packet);
        bbr_network_ctrl_update_t   bbr_create_rate_update(bbr_controller_t* bbr, int64_t at_time);
        void                        rate_stat_update(rate_stat_t* rate, size_t count, int64_t now_ts);
        void                        rate_stat_erase(rate_stat_t* rate, int64_t now_ts);
        int                         rate_stat_rate(rate_stat_t* rate, int64_t now_ts);
        int32_t                     bbr_bandwidth_estimate(bbr_controller_t* bbr);
        int32_t                     bbr_feedback_get_bitrate(bbr_fb_adapter_t* adapter);

        //带宽滤波器
        void                        wnd_filter_init(windowed_filter_t* filter, int64_t wnd_size, compare_func_f comp);
        void                        wnd_filter_reset(windowed_filter_t* filter, int64_t new_sample, int64_t new_ts);
        int64_t                     wnd_filter_best(windowed_filter_t* filter);
        void                        wnd_filter_update(windowed_filter_t* filter, int64_t new_sample, int64_t new_ts);
        int64_t                     wnd_filter_third_best(windowed_filter_t* filter);

        //BBR状态转化
        inline int                  bbr_in_recovery(bbr_controller_t* bbr);
        void                        bbr_enter_startup_mode(bbr_controller_t* bbr);
        void                        bbr_enter_probe_bandwidth_mode(bbr_controller_t* bbr, int64_t now_ts);
        int                         bbr_is_probing_for_more_bandwidth(bbr_controller_t* bbr);
        int                         bbr_update_bandwidth_and_min_rtt(bbr_controller_t* bbr, int64_t now_ts, int64_t  fb_send_time, shared_ptr<bbr_packet_point_t> point, uint32_t bandwidth);
        int							bbr_should_extend_min_rtt_expiry(bbr_controller_t* bbr);
        void                        bbr_update_recovery_state(bbr_controller_t* bbr, int64_t last_acked_packet, int losses, int is_round_start);
        void                        bbr_update_ack_aggregation_bytes(bbr_controller_t* bbr, int64_t ack_time, size_t newly_acked_bytes);
        void                        bbr_update_gain_cycle_phase(bbr_controller_t* bbr, int64_t now_ts, size_t prior_in_flight, bool losses);
        void                        bbr_check_if_full_bandwidth_reached(bbr_controller_t* bbr);

        bool                        bbr_check_probe_bw_if_full_bandwidth_reached(bbr_controller_t* bbr, size_t bytes_inflight);
        void                        bbr_maybe_exit_startup_or_drain(bbr_controller_t* bbr, bbr_feedback_t* feedback);
        void                        bbr_maybe_enter_or_exit_probe_rtt(bbr_controller_t* bbr, bbr_feedback_t* feedback, int is_round_start, int min_rtt_expired);

        //pacing_rate,congestion_window计算
        void                        bbr_calculate_pacing_rate(bbr_controller_t* bbr, uint32_t bandwidth);
        void                        bbr_calculate_congestion_window(bbr_controller_t* bbr, size_t bytes_acked);
        void                        bbr_calculate_recovery_window(bbr_controller_t* bbr, size_t bytes_acked, size_t bytes_lost, size_t bytes_in_flight);

        //拥塞窗口
        int32_t                     bbr_get_congestion_window(bbr_controller_t* bbr);
        size_t                      bbr_probe_rtt_congestion_window(bbr_controller_t* bbr);
        size_t                      bbr_get_target_congestion_window(bbr_controller_t* bbr, double gain);

        //pacing_rate
        int32_t                     bbr_pacing_rate(bbr_controller_t* bbr);
        double                      bbr_get_pacing_gain(bbr_controller_t* bbr, int index);

        //往返时延
        int64_t	                    bbr_initial_rtt_us(bbr_rtt_stat_t* s);
        int64_t                     bbr_get_min_rtt(bbr_controller_t* bbr);
        void                        bbr_rtt_update(bbr_rtt_stat_t* s, int64_t send_delta, int64_t ack_delay, int64_t now_ts);

        //丢包的处理
        double                      bbr_loss_filter_get(bbr_loss_rate_filter_t* filter);
        int                         bbr_feedback_get_loss(bbr_feedback_t* feedback, bbr_packet_info_t* loss_arr, int arr_size);
        void                        bbr_discard_lost_packets(bbr_controller_t* bbr, bbr_packet_info_t packets[], int size);
        void                        sampler_on_packet_lost(bbr_bandwidth_sampler_t* sampler, int64_t packet_number);
        void                        bbr_loss_filter_update(bbr_loss_rate_filter_t* filter, int64_t feedback_ts, int packets_sent, int packets_lost);

        //反馈包处理器
        void                        bbr_feedback_on_feedback(bbr_fb_adapter_t* adapter, bbr_feedback_msg_t* msg);
        int                         sender_history_get(sender_history_t* hist, uint16_t seq, packet_feedback* packet, int remove_flag);
        bbr_network_ctrl_update_t   bbrOnFeedback(bbr_controller_t* BBR, bbr_feedback_t* FeedBack, uint32_t bandwidth);
        int                         bbr_feedback_get_received(bbr_feedback_t* feedback, bbr_packet_info_t* recvd_arr, int arr_size);
        void                        bbr_on_network_invalidation();

        //ROUND TRIP判断
        int                         bbr_update_round_trip_counter(bbr_controller_t* bbr, int64_t last_acked_packet);


    private:
        const DWORD m_d_self_tid;	//本端的终端号
        const DWORD m_d_remote_tid;	//对端的终端号
        const DWORD hop_count = GetIntegerKeyIni("QOS", "Hop_count", 1);	//获取发送接收经过跳数
        const bool IsSatellite = GetBoolValueKeyIni("Display", "IsSatellite", false);
        const bool IsMobile = GetBoolValueKeyIni("Display", "IsMobile", false);
        const bool IsBroadband = GetBoolValueKeyIni("Display", "IsBroadband", false);
        int gaincyclelength = kGainCycleLength - rand() % bbr_cycle_rand;
        int cycle_len_ = { 0 };
        int index_ = 0;//增益系数数组索引值
        bool IsStart = false;
        DWORD LastTime;
        int64_t cycle_mstamp_{ GET_SYS_TIME() };
        queue<shared_ptr<const SelfToRemoteQosInfo>  > m_self_to_remote_qos_info_queue;	//本端发送给对端的数据包被NetCombTransfer偷窥后反馈给本模块的包序号和时间戳等信息所组成的队列
        //queue< SelfToRemoteQosInfo > m_self_to_remote_qos_info_queue;
        //deque<shared_ptr<const SelfToRemoteQosInfo>  > m_self_to_remote_qos_info_queue;
        queue<shared_ptr<const Remote2SelfPacketQosFeedbackPacket> > m_remote_to_self_feedback_packet_queue;	//对端发送给本端的反馈包队列
        bbr_fb_adapter_t			feedback;
        //queue< Remote2SelfPacketQosFeedbackPacket > m_remote_to_self_feedback_packet_queue;

        int  feedback_packet_count=0;
        queue < shared_ptr < feedback_ack_message> > ack_message_before_n_feedback;
        pthread_mutex_t m_cs;	//临界区变量

        //处理数据输入1
        void DealSelfToRemoteQosInfo();
        //处理数据输入2
        void DealRemoteToSelfFeedbackPacket();
    };
}