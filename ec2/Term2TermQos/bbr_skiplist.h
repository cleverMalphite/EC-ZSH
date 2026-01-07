#pragma once

#ifndef __CF_SKIPLIST_H_
#define __CF_SKIPLIST_H_

#include <stdint.h>
#include <sys/time.h>
#include <iostream>

#define SU_MAX(a, b)		((a) > (b) ? (a) : (b))
#define SU_MIN(a, b)		((a) < (b) ? (a) : (b))
#define SU_ABS(a, b)		((a) > (b) ? ((a) - (b)) : ((b) - (a)))
#define SKIPLIST_MAXDEPTH	8
#define MAX_BBR_FEEDBACK_COUNT   20000
#define DEFAULT_PACKET_SIZE 1024
#define GET_SYS_TIME()  su_get_sys_time() / 1000

#define k_loss_statistics_window_ms 4000
#define k_max_statistics_window_number 200

#define PacketSeqloopSize   65530
//#define k_bbr_heartbeat_timer  30
#define k_bbr_heartbeat_timer  5
#define MAX_SEND_BITRATE (100000000000 * 8 * 1000)   //100000kbps*8000
#define MIN_SEND_BITRATE (5 * 8 * 1000)
#define START_SEND_BITRATE (140 * 8 * 1000)
#define kInitialCongestionWindowPackets 4
// The minimum CWND to ensure delayed acks don't reduce bandwidth measurements.
// Does not inflate the pacing rate.
#define kDefaultMinCongestionWindowPackets 2
#define kDefaultMaxCongestionWindowPackets 9000   //拥塞窗口最大约108Mb
#define kDefaultTCPMSS 1500
// Constants based on TCP defaults.
#define kMaxSegmentSize kDefaultTCPMSS
#define packet_size 1500
// The length of the gain cycle.
#define kGainCycleLength 8
// The size of the bandwidth filter window, in round-trips.
#define kBandwidthWindowSize (kGainCycleLength + 22)
#define k_rate_window_size 1000
#define k_rate_scale 8000
#define kInitialBandwidthKbps 300
// Congestion window gain for QUIC BBR during PROBE_BW phase.
#define kProbeBWCongestionWindowGain 2.0f
#define kBbrRttVariationWeight 0.0f
#define k_min_pace_bitrate (10*1000)
#define k_history_cache_ms		5000
#define  kAlpha 0.125
#define  kOneMinusAlpha (1 - kAlpha)
#define  kBeta 0.25
#define  kOneMinusBeta (1 - kBeta)
#define  kGamma    0.5
#define kMaxTrackedPackets  500000
#define kUpdateIntervalMs	2000
#define kMinRttExpiry 10000   ///////////////由100000改为1000
#define kLimitNumPackets	50
#define kSimilarMinRttThreshold 1.125
#define kStartupGrowthTarget 1.25
#define kTake_sample_bandwidth_threshold 0.9
#define kTake_sample_bandwidth_threshold_upper 1.1
// The gain used for the slow start, equal to 4ln(2).
#define kHighGain 2.77f
#define bbr_cycle_rand  7
// The gain used to drain the queue after the slow start.
#define kDrainGain  (1.f / kHighGain)
#define kMaxPacketSize 1500
// The minimum time the connection can spend in PROBE_RTT mode.
#define kProbeRttTimeMs 200///////1m一下500比较合适 10M以上一般200合适
// The gain used in STARTUP after loss has been detected.
// 1.5 is enough to allow for 25% exogenous loss and still observe a 25% growth
// in measured bandwidth.
#define kStartupAfterLossGain 1.5
#define kDefaultPoints		2000000

#define kFeedBacknumber     20
#define kRTT_congestion_threshold  3
#define krRecv_Block_Stop_Sampling_num 40

/*bbr状态*/
enum {
    /*Startup phase of the connection*/
    STARTUP,
    /*After achieving the highest possible bandwidth during the startup, lower the pacing rate in order to drain the queue*/
    DRAIN,
    /*Cruising mode*/
    PROBE_BW,
    /*Temporarily slow down sending in order to empty the buffer and measure the real minimum RTT*/
    PROBE_RTT
};
/* Indicates how the congestion control limits the amount of bytes in flight*/
enum {
    /*Do not limit*/
    NOT_IN_RECOVERY,
    /*Allow an extra outstanding byte for each byte acknowledged*/
    CONSERVATION,
    /*Allow 1.5 extra outstanding bytes for each byte acknowledged*/
    MEDIUM_GROWTH,
    /*Allow two extra outstanding bytes for each byte acknowledged (slow start)*/
    GROWTH
};


int	max_val_func(int64_t v1, int64_t v2);
int64_t su_get_sys_time();
typedef struct
{
    int64_t			create_ts;			/*创建时间戳*/
    int64_t			arrival_ts;			/*到达时间戳*/
    int64_t			send_ts;			/*发送时间戳*/

    uint16_t		sequence_number;	/*发送通道的报文序号*/
    size_t			payload_size;		/*包数据大小*/
}packet_feedback;

typedef struct
{
    int64_t			latest_rtt;
    int64_t			min_rtt;
    int64_t			smoothed_rtt = 0;
    int64_t			previous_srtt;

    int64_t			mean_deviation;
    int64_t			initial_rtt_us;
}bbr_rtt_stat_t;

typedef struct
{
    int			lost_count;
    int			total_count;
    double		loss_rate_estimate;
    int64_t		next_loss_update_ms;
}bbr_loss_rate_filter_t;

typedef struct
{
    int64_t		at_time;
    int32_t		min_rate;
    int32_t		max_rate;
}bbr_target_rate_constraint_t;

typedef struct
{
    int64_t			sample;
    int64_t			ts;
}wnd_sample_t;

typedef struct
{
    int64_t			send_time;
    size_t			size;

    size_t			total_data_sent;
    size_t			total_data_acked_at_the_last_recv_fbpacket;
    size_t			total_data_sent_at_last_acked_packet;
    int             last_acked_packet_is_app_limited;
    int64_t			last_acked_packet_sent_time;
    int64_t			last_acked_packet_ack_time;

    int				is_app_limited;
    int				ignore;

    int             is_recv_burst;
    size_t          total_data_acked_at_the_last_acked_packet_before_n_feedback;
    int64_t         last_acked_packet_before_n_feedback_ack_time;
}bbr_packet_point_t;


typedef union {
    int8_t		i8;
    uint8_t		u8;
    int16_t		i16;
    uint16_t	u16;
    int32_t		i32;
    uint32_t	u32;
    int64_t		i64;
    uint64_t	u64;
    void*		ptr;
    /*custom context, such as: string*/
}skiplist_item_t;

/*skiplist iterator*/
typedef struct wb_skiplist_iter_s
{
    skiplist_item_t key;
    skiplist_item_t val;

    int	depth;

    struct wb_skiplist_iter_s* next[0];

}skiplist_iter_t;

/*key compare callback*/
typedef int(*skiplist_compare_f)(skiplist_item_t k1, skiplist_item_t k2);
/*free k/v pair callback*/
typedef void(*skiplist_free_f)(skiplist_item_t key, skiplist_item_t val, void* args);

typedef struct
{
    size_t				size;

    skiplist_compare_f	compare_fun;
    skiplist_free_f		free_fun;
    void*				args;					/*free句柄*/
    skiplist_iter_t*	entries[SKIPLIST_MAXDEPTH];
}skiplist_t;

void free_packet_feedback(skiplist_item_t key, skiplist_item_t val, void* args);
skiplist_t*				skiplist_create(skiplist_compare_f compare_cb, skiplist_free_f free_cb, void* args);
void					skiplist_destroy(skiplist_t* sl);

void					skiplist_clear(skiplist_t* sl);
size_t					skiplist_size(skiplist_t* sl);

void					skiplist_insert(skiplist_t* sl, skiplist_item_t key, skiplist_item_t val);
skiplist_iter_t*		skiplist_remove(skiplist_t* sl, skiplist_item_t key);

skiplist_iter_t*		skiplist_search(skiplist_t* sl, skiplist_item_t key);
skiplist_iter_t*		skiplist_first(skiplist_t* sl);

/*traverse skiplist*/
#define SKIPLIST_FOREACH(sl, iter)	\
	for(iter = sl->entries[0]; iter != NULL; iter = iter->next[0])

int						id8_compare(skiplist_item_t k1, skiplist_item_t k2);
int						id16_compare(skiplist_item_t k1, skiplist_item_t k2);
int						id32_compare(skiplist_item_t k1, skiplist_item_t k2);
int						id64_compare(skiplist_item_t k1, skiplist_item_t k2);

int						idu8_compare(skiplist_item_t k1, skiplist_item_t k2);
int						idu16_compare(skiplist_item_t k1, skiplist_item_t k2);
int						idu32_compare(skiplist_item_t k1, skiplist_item_t k2);
int						idu64_compare(skiplist_item_t k1, skiplist_item_t k2);

#endif