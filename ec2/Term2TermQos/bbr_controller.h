#pragma once

#include<stdint.h>
#include"bbr_skiplist.h"
#include"bbr_circlequeue.h"
#include "bbr_sendhistory_circle_queue.h"
/*接收端统计丢包*/
typedef struct
{
    int64_t			stat_ts;
    int64_t        prev_max_id;          /*上一次统计的最大序号*/
    int64_t		   max_id;				/*接收到最大序号*/
    int            count;               /*本次接收的报文个数*/

}cc_loss_statistics_t;

typedef struct bin_stream_s
{
    uint8_t*	data;
    size_t		size;

    uint8_t*	rptr;
    size_t		rsize;

    uint8_t*	wptr;
    size_t		used;

    uint64_t	magic;
}bin_stream_t;

/*接收端类定义*/
typedef struct
{
    uint16_t				seq;
    //uint16_t				delta_ts;
}bbr_sampler_t;

typedef struct
{
    uint8_t                 feedback_index;    //反馈包标志位为3
    int32_t                 nDeviceID;
    int32_t                 nRemoteID;
    uint8_t					flag;
    /*loss info msg*/
    float					fraction_loss;
    uint8_t                  loss_rate;     //丢包率
    int						loss_num;       //丢包数
    int                     packet_num;     //接收端收到数据包数
    int                     packetseqroundtrip;    //数据包序号轮数
    uint32_t                accumulated_count;     //接收端累积确认数据量
    /*proxy_ts_msg*/
    uint32_t                acked_bitrate;         //接收端平均接收速率
    int				        last_num;              //接收端接收到的最后一个包序号
    int64_t                 last_num_recv_time;    //接收最后一个包序号的发送时间
    int64_t                feedbackpacketsendtime;
    int                     delta_ts;              //接收端累积停留时间
    int                     sampler_num;           //
    bbr_sampler_t			samplers[MAX_BBR_FEEDBACK_COUNT];
}bbr_feedback_msg_t;

/*定义一个通过记录sent和acked报文轨迹来统计带宽的跟踪器*/
typedef struct
{
    int32_t				rate_bps;
    size_t				total_data_sent;
    size_t				total_data_acked;
    size_t				total_data_sent_at_last_acked_packet;

    int64_t				last_acked_packet_sent_time;
    int64_t				last_acked_packet_ack_time;
    int64_t				last_sent_packet;
    int                 last_acked_packet_app_limited;//新增，过滤掉上一次ack数据包为app_limited数据包的带宽采样值，防止计算sendrate时偏小，造成计算带宽值的大幅度下降。
    int					is_app_limited;
    int                 is_recv_burst;
    int64_t				end_of_app_limited_phase;

    int					size;    //发送记录表的大小
    int					count;   //发送记录表中元素个数
    int64_t				start_pos; //发送记录表中有效元素开始位置
    int64_t				index;      //发送记录表当前发送到的位置序号
    int64_t             roundtrip;
    //bbr_packet_point_t*	points;
    SendCircularDataQueue*  points;
}bbr_bandwidth_sampler_t;

typedef int(*compare_func_f)(int64_t v1, int64_t v2);

typedef struct
{
    int64_t			wnd_size;
    wnd_sample_t	estimates[3];
    compare_func_f  comp_func;
}windowed_filter_t;

/*BBR的配置项目*/
typedef struct
{
    double probe_bw_pacing_gain_offset;
    double encoder_rate_gain;
    double encoder_rate_gain_in_probe_rtt;

    /* RTT delta to determine if startup should be exited due to increased RTT.*/
    int64_t exit_startup_rtt_threshold_ms;

    size_t initial_congestion_window;
    size_t min_congestion_window;
    size_t max_congestion_window;

    double probe_rtt_congestion_window_gain;
    double real_probe_rtt_congestion_window_gain;
    int	pacing_rate_as_target;

    /* Configurable in QUIC BBR:*/
    int exit_startup_on_loss;
    /* The number of RTTs to stay in STARTUP mode.  Defaults to 3.*/
    int64_t num_startup_rtts;

    int64_t num_probebw_inflight_grows;
    /* When true, recovery is rate based rather than congestion window based.*/
    int rate_based_recovery;
    double max_aggregation_bytes_multiplier;
    /* When true, pace at 1.5x and disable packet conservation in STARTUP.*/
    int slower_startup;
    /* When true, disables packet conservation in STARTUP.*/
    int rate_based_startup;
    /* If true, will not exit low gain mode until bytes_in_flight drops below BDP or it's time for high gain mode.*/
    int fully_drain_queue;
    /* Used as the initial packet conservation mode when first entering recovery.*/
    int initial_conservation_in_startup;

    double max_ack_height_window_multiplier;
    /* If true, use a CWND of 0.75*BDP during probe_rtt instead of 4 packets.*/
    int probe_rtt_based_on_bdp;
    /* If true, skip probe_rtt and update the timestamp of the existing min_rtt
      to now if min_rtt over the last cycle is within 12.5% of the current
     min_rtt. Even if the min_rtt is 12.5% too low, the 25% gain cycling and
     2x CWND gain should overcome an overly small min_rtt.*/
    int probe_rtt_skipped_if_similar_rtt;
    /*If true, disable PROBE_RTT entirely as long as the connection was recently app limited.*/
    int probe_rtt_disabled_if_app_limited;
}bbr_config_t;

typedef struct
{
    bbr_rtt_stat_t					rtt_stat;						/*rtt延迟平滑计算模块*/
    bbr_loss_rate_filter_t			loss_rate;						/*丢包计算*/
    bbr_target_rate_constraint_t	constraints;

    int								mode;							/*bbr模式状态*/
    bbr_bandwidth_sampler_t*		sampler;						/*带宽计算模型，基于发送和ACK两个维度进行计算*/
    int64_t							round_trip_count;
    int64_t                         round_trip_countprevious;       ///新增用于RTT状态分阶段降速
    int64_t							last_sent_packet;
    int64_t							current_round_trip_end;

    int32_t							default_bandwidth;
    windowed_filter_t				max_bandwidth;

    windowed_filter_t				max_ack_height;


    int64_t							aggregation_epoch_start_time;
    size_t							aggregation_epoch_bytes;
    size_t							bytes_acked_since_queue_drained;
    double							max_aggregation_bytes_multiplier;

    int64_t							min_rtt;
    int64_t							last_rtt;
    int64_t                         last_PROBE_RTT_minrtt;
    int64_t							min_rtt_timestamp;

    size_t							congestion_window;
    size_t							initial_congestion_window;
    size_t							max_congestion_window;
    size_t							min_congestion_window;

    int32_t							pacing_rate;
    double							pacing_gain;
    double							congestion_window_gain;
    double							congestion_window_gain_constant;

    double							rtt_variance_weight;

    int								cycle_current_offset;
    int64_t							last_cycle_start;

    int								is_at_full_bandwidth;

    int                            is_probe_bw_bandwidth_full;

    int64_t							rounds_without_bandwidth_gain;

    int64_t							probe_bw_rounds_without_inflight_grow;
    int64_t                         rounds_with_largelossrate;
    int32_t							bandwidth_at_last_round;

    int								exiting_quiescence;
    int64_t							exit_probe_rtt_at;
    int								probe_rtt_round_passed;

    int								last_sample_is_app_limited;

    int								recovery_state;

    int64_t							end_recovery_at;

    size_t							recovery_window;

    bbr_config_t					config;

    int								app_limited_since_last_probe_rtt;
    int64_t							min_rtt_since_last_probe_rtt;

}bbr_controller_t;

typedef struct
{
    int64_t			at_time;
    size_t			data_window;
    int64_t			time_window;
    size_t			pad_window;
}bbr_pacer_config_t;

typedef struct
{
    int64_t			at_time;
    //int32_t			bandwidth;
    int64_t			bandwidth;
    int64_t			rtt;
    int64_t			bwe_period;
    float			loss_rate_ratio;

    //int32_t			target_rate;
    int64_t target_rate;
}bbr_target_transfer_rate_t;

typedef struct
{
    size_t						congestion_window;
    bbr_pacer_config_t			pacer_config;
    bbr_target_transfer_rate_t  target_rate;
}bbr_network_ctrl_update_t;

typedef struct
{
    int64_t		send_time;
    int64_t		recv_time;
    size_t		size;
    int64_t		seq;
    size_t		data_in_flight;
}bbr_packet_info_t;

typedef struct
{
    int64_t					feedback_time;
    int64_t					Fb_Send_Time;
    size_t					data_in_flight;
    size_t					prior_in_flight;

    int						packets_num;
    int                     loss_num;
    float                 loss_rate;
    int                     last_num;
    int                     packetseqroundtrip;
    uint32_t                data_acked;
    int64_t                 time_delta;
    bbr_packet_info_t		packets[MAX_BBR_FEEDBACK_COUNT];
}bbr_feedback_t;

typedef struct
{
    int			sum;
    int			sample;
}rate_bucket_t;

/*单位时间带宽统计器*/
typedef struct
{
    int64_t			oldest_ts;
    int				oldest_index;

    rate_bucket_t*	buckets;
    int				wnd_size;

    float			scale;

    int				accumulated_count;
    int				sample_num;
}rate_stat_t;

typedef struct
{
    uint32_t		limited_ms;

    //cf_unwrapper_t	wrapper;
    int64_t			last_ack_seq_num;
    int64_t         last_ack_seq_sendtime;
    CircularDataQueue		*l;
    //size_t			outstanding_bytes;
    long long outstanding_bytes;
}sender_history_t;
typedef struct
{
    sender_history_t*		hist;
    bbr_feedback_t			feedback;
    rate_stat_t				acked_bitrate;
}bbr_fb_adapter_t;

typedef struct
{
    int32_t		bandwidth;
    int64_t		rtt;
    int			is_app_limited;
    int         is_recv_burst;
}bbr_bandwidth_sample_t;

typedef struct {
    size_t				total_data_acked;
    int64_t				last_acked_packet_ack_time;
}feedback_ack_message;