
#include "SpeedControlApi.h"
#include "MRUDPApi.h"
#include "SelfToRemoteQosUnit.h"
#include"bbr_circlequeue.h"
#include <cmath>

namespace Term2TermQos {
    extern pRttCallBackSimple g_qos_rtt_callback;

    SelfToRemoteQosUnit::SelfToRemoteQosUnit(DWORD d_self_tid, DWORD d_remote_tid) : m_d_self_tid(d_self_tid),
                                                                                     m_d_remote_tid(d_remote_tid) {
        pthread_mutex_init(&m_cs, nullptr);
        ///在此处进行发送端初始化
        bbr = nullptr;
        encoding_rate_ratio = 1.0f;
        notify_ts = -1;
        info.congestion_window = -1;
        target_bitrate = k_min_pace_bitrate;
        bbr_feedback_adapter_init(&feedback);
        bbr_target_rate_constraint_t co;
        max_bitrate = MAX_SEND_BITRATE;      //最大发送速率定义为100Mbps
        min_bitrate = MIN_SEND_BITRATE;
        target_bitrate = START_SEND_BITRATE * 1.07f * (1500 / 40.0);//数据包总长/包头长度
        if (bbr == NULL) {
            co.at_time = GetTickCount();
            co.min_rate = MIN_SEND_BITRATE / 8000;   //单位kbps
            co.max_rate = MAX_SEND_BITRATE / 8000;   //单位kbps

            bbr = bbr_create(&co, (START_SEND_BITRATE * 1.07f * (1500 / 40.0)) / 8000.0);    //包长/包头
        }
        //printf("[OOMDebug]::Self2RemoteQOSUnit Create\n");
    }

    //ToDo MisMatch free() / delete / delete[]
    SelfToRemoteQosUnit::~SelfToRemoteQosUnit() {
        //printf("\n~SelfToRemoteQosUnit\n");
        pthread_mutex_destroy(&m_cs);
        bbr_feedback_adapter_destroy(&feedback);
        //销毁发送端
        if (bbr != nullptr) {
            bbr_destroy(bbr);
            bbr = nullptr;
        }
        //printf("[OOMDebug]::Self2RemoteQOSUnit Destroy\n");
    }

    void SelfToRemoteQosUnit::RecvSelfToRemoteQosInfo(
            const shared_ptr<const SelfToRemoteQosInfo> &p_self_to_remote_qos_info) {
        MutexLockGuard gGuard(m_cs);    //临界区

        if (nullptr == p_self_to_remote_qos_info) {
            return;
        }
        m_self_to_remote_qos_info_queue.push(p_self_to_remote_qos_info);
    }

    void SelfToRemoteQosUnit::RecvRemote2SelfPacketQosFeedbackPacket(
            shared_ptr<const Remote2SelfPacketQosFeedbackPacket> p_feedback_packet) {
        MutexLockGuard gGuard(m_cs);


        if (nullptr == p_feedback_packet) {
            return;
        }
        m_remote_to_self_feedback_packet_queue.push(p_feedback_packet);
    }

    void SelfToRemoteQosUnit::DoCycleTime() {
        DealSelfToRemoteQosInfo();
        //printf("Return Form DealSelfToRemoteQosInfo\n");
        DealRemoteToSelfFeedbackPacket();
    }

/*
#ifdef DEAL_CAPACITY_DealSelfToRemoteQosInfo
    string DealSelfToRemoteQosInfo_unit_name("DealSelfToRemoteQosInfo");
    DWORD DealSelfToRemoteQosInfo_unit_cycle = 10000;
    void DealSelfToRemoteQosInfo(string name, DWORD statics)
    {
        std::cout << name << ":" << statics << std::endl;;
    }
    StatisticsCapacity DealSelfToRemote_statistics(DealSelfToRemoteQosInfo_unit_name, DealSelfToRemoteQosInfo_unit_cycle, DealSelfToRemoteQosInfo);
#endif // DEAL_CAPACITY_DealSelfToRemoteQosInfo
*/

    void SelfToRemoteQosUnit::DealSelfToRemoteQosInfo() {
        //printf("Run SelfToRemoteQosUnit::DealSelfToRemoteQosInfo()1\n");
        MutexLockGuard gGuard(m_cs);
        //TODO ADD
        //处理相关消息，给出反馈，注意在发送反馈函数之前退出临界区，避免死锁
        bbr_packet_info_t info;
        packet_feedback packet;
        int64_t now_ts = GET_SYS_TIME();
        //printf("Run SelfToRemoteQosUnit::DealSelfToRemoteQosInfo()2\n");
        size_t s = m_self_to_remote_qos_info_queue.size();
        if (s == 0) { return; }
        //printf("Run SelfToRemoteQosUnit::DealSelfToRemoteQosInfo()3\n");
        shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_msg = m_self_to_remote_qos_info_queue.front();
/*
#ifdef DEAL_CAPACITY_DealSelfToRemoteQosInfo
        DealSelfToRemote_statistics.ComputeOnce();
#endif // DEAL_CAPACITY_DealSelfToRemoteQosInfo
*/
        m_self_to_remote_qos_info_queue.pop();//删除一个队列信息
        packet.arrival_ts = -1;
        packet.create_ts = packet.send_ts = p_self_to_remote_msg->m_d_timestamp;
        packet.payload_size = p_self_to_remote_msg->m_d_data_length;
        packet.sequence_number = p_self_to_remote_msg->m_d_index;
        info.data_in_flight = feedback.hist->outstanding_bytes;
        //将数据包信息存进跳表中
        //printf("Run SelfToRemoteQosUnit::DealSelfToRemoteQosInfo()4\n");
        sender_history_add(feedback.hist, &packet);
        //printf("Run SelfToRemoteQosUnit::DealSelfToRemoteQosInfo()\n");
        info.send_time = p_self_to_remote_msg->m_d_timestamp;
        info.recv_time = -1;
        info.size = p_self_to_remote_msg->m_d_data_length;
        info.seq = p_self_to_remote_msg->m_d_index;
        //对数据包进行记录
        bbr_on_send_packet(bbr, &info);
    }

    void SelfToRemoteQosUnit::DealRemoteToSelfFeedbackPacket() {
        //printf("Run SelfToRemoteQosUnit::DealRemoteToSelfFeedbackPacket()\n");
        MutexLockGuard gGuard(m_cs);
        uint32_t Acked_bitrate;
        bbr_feedback_msg_t msg;
        int64_t now_ts = GET_SYS_TIME();
        size_t s = m_remote_to_self_feedback_packet_queue.size();
        if (s == 0) return;
        shared_ptr<const Remote2SelfPacketQosFeedbackPacket> p_remote_to_self_feedback = m_remote_to_self_feedback_packet_queue.front();
        if (p_remote_to_self_feedback->GetFlag() == 1)   //说明有丢失信息
            m_fraction_loss = p_remote_to_self_feedback->GetLoss();
        feedback.feedback.loss_num = p_remote_to_self_feedback->GetLossNum();
        feedback.feedback.loss_rate = p_remote_to_self_feedback->GetLoss();
        feedback.feedback.packets_num = p_remote_to_self_feedback->GetPacketNum();
        feedback.feedback.last_num = p_remote_to_self_feedback->GetLastNum();
        feedback.feedback.packetseqroundtrip = p_remote_to_self_feedback->Getpacketseqroundtrip();
        feedback.feedback.data_acked = p_remote_to_self_feedback->GetData_acked();
        feedback.feedback.feedback_time = p_remote_to_self_feedback->GetRecvTime() -
                                          (p_remote_to_self_feedback->GetFb_Send_Time() -
                                           p_remote_to_self_feedback->GetLastNumSendTime());//减去接收端处理时延
        feedback.feedback.Fb_Send_Time = p_remote_to_self_feedback->GetFb_Send_Time();
//        printf("[QOSDebug]::QOSFeedBack lost number:%d\n",feedback.feedback.loss_num);
//        printf("[QOSDebug]::QOSFeedBack lost rate:%f\n",feedback.feedback.loss_rate);
//        printf("[QOSDebug]::QOSFeedBack last number:%d\n",feedback.feedback.last_num);
//        printf("[QOSDebug]::QOSFeedBack Feed BackTime calculate = recvtime:%ld - fb_send_time:%ld + last_num_recv_time:%ld\n",p_remote_to_self_feedback->GetRecvTime(),
//               p_remote_to_self_feedback->GetFb_Send_Time(),p_remote_to_self_feedback->GetLastNumSendTime());
//        printf("[QOSDebug]::QOSFeedBack Feed BackTime:%d\n\n",feedback.feedback.feedback_time);

        //计算data_in_flight
        feedback.feedback.time_delta = now_ts - p_remote_to_self_feedback->GetLastNumSendTime();
        msg.last_num = p_remote_to_self_feedback->GetLastNum();
        msg.delta_ts = p_remote_to_self_feedback->GetDeltaTs();
        msg.feedbackpacketsendtime = p_remote_to_self_feedback->GetFb_packet_sendts();
        msg.acked_bitrate = p_remote_to_self_feedback->GetRecvRate();
        msg.accumulated_count = p_remote_to_self_feedback->GetData_acked();
        payload_size = msg.delta_ts * msg.acked_bitrate; //单位:字节
        feedback.feedback.data_in_flight = feedback.hist->outstanding_bytes;

        bbr_feedback_on_feedback(&feedback, &msg);

        if (bbr != nullptr) {
            Acked_bitrate = p_remote_to_self_feedback->GetRecvRate();
            /*printf("Address:%x\n",this);
            printf("BBR Add:%x\n",bbr);*/
            info = bbrOnFeedback(bbr, &feedback.feedback, Acked_bitrate);
            bbr_on_network_invalidation();
        }

        m_remote_to_self_feedback_packet_queue.pop();
    }





    ///BBR发送端的函数初始化
    /*int32_t SelfToRemoteQosUnit::bin_stream_init(void* ptr)
    {
        bin_stream_t* strm;

        if (big_endian == -1) {
            union
            {
                uint16_t	s16;
                uint8_t		s8[2];
            }un;

            un.s16 = 0x0100;
            if (un.s8[0] == 0x01)
                big_endian = 1;
            else
                big_endian = 0;
        }

        strm = (bin_stream_t *)ptr;
        strm->size = sizeof(uint8_t) * DEFAULT_PACKET_SIZE;
        strm->data = (uint8_t *)malloc(strm->size);
        if (strm->data == NULL)
            return -1;

        strm->rptr = strm->data;
        strm->wptr = strm->data;

        strm->rsize = 0;
        strm->used = 0;

        strm->magic = 0;

        return 0;
    }*/
    void SelfToRemoteQosUnit::bbr_feedback_adapter_init(bbr_fb_adapter_t *adapter) {
        adapter->hist = sender_history_create(k_history_cache_ms);
        adapter->feedback.data_in_flight = 0;
        adapter->feedback.prior_in_flight = 0;
        adapter->feedback.feedback_time = GetTickCount();
        adapter->feedback.packets_num = 0;
        rate_stat_init(&adapter->acked_bitrate, k_rate_window_size, k_rate_scale);
    }

    void SelfToRemoteQosUnit::rate_stat_init(rate_stat_t *rate, int wnd_size, float scale) {
        rate->wnd_size = wnd_size;
        rate->scale = scale;

        rate->buckets = (rate_bucket_t *) calloc(wnd_size, sizeof(rate_bucket_t));
        rate->accumulated_count = 0;
        rate->sample_num = 0;

        rate->oldest_index = 0;
        rate->oldest_ts = -1;
    }

    //TODO:Leak_DefinitelyLost
    sender_history_t *SelfToRemoteQosUnit::sender_history_create(uint32_t limited_ms) {
        sender_history_t *hist = (sender_history_t *) calloc(1, sizeof(sender_history_t));
        hist->limited_ms = limited_ms;
        hist->l = new CircularDataQueue(65530);
        //printf("[OOMDebug]::New Send_History_T *, Return this Pointer:%d\n", hist);
        return hist;
    }

    bbr_controller_t *SelfToRemoteQosUnit::bbr_create(bbr_target_rate_constraint_t *co, int32_t starting_bandwidth) {
        bbr_controller_t *bbr = (bbr_controller_t *) calloc(1, sizeof(bbr_controller_t));
        /*初始化RTT统计模块*/
        bbr->rtt_stat.latest_rtt = 0;
        bbr->rtt_stat.min_rtt = 0;
        //bbr->rtt_stat.smoothed_rtt = 100;
        bbr->rtt_stat.smoothed_rtt = 0;
        bbr->rtt_stat.previous_srtt = 20;
        bbr->rtt_stat.mean_deviation = 20;
        bbr->rtt_stat.initial_rtt_us = 1000 * 100;  //kNumMicrosPerMilli * kInitialRttMs

        /*初始化BBR默认配置参数*/
        bbr->config.probe_bw_pacing_gain_offset = 0.25;
        bbr->config.encoder_rate_gain = 1;
        bbr->config.encoder_rate_gain_in_probe_rtt = 1;
        bbr->config.exit_startup_rtt_threshold_ms = 100000000;/*设置一个很大的值作为初始值，意思是这个值不做判断*/
        bbr->config.initial_congestion_window = kInitialCongestionWindowPackets * kDefaultTCPMSS;
        bbr->config.max_congestion_window = kDefaultMaxCongestionWindowPackets * kDefaultTCPMSS;
        bbr->config.min_congestion_window = kDefaultMinCongestionWindowPackets * kDefaultTCPMSS;
        bbr->config.probe_rtt_congestion_window_gain = 0.8;
        bbr->config.real_probe_rtt_congestion_window_gain = 0.75;
        //bbr->config.probe_rtt_congestion_window_gain = 1;
        bbr->config.pacing_rate_as_target = true;
        bbr->config.exit_startup_on_loss = true;
        bbr->config.num_startup_rtts = 5;

        bbr->config.num_probebw_inflight_grows = 5;

        bbr->config.rate_based_recovery = false;
        bbr->config.max_aggregation_bytes_multiplier = 0;
        bbr->config.slower_startup = false;
        bbr->config.rate_based_startup = false;
        bbr->config.fully_drain_queue = false;
        bbr->config.initial_conservation_in_startup = CONSERVATION;
        bbr->config.max_ack_height_window_multiplier = 1;
        bbr->config.probe_rtt_based_on_bdp = true;
        bbr->config.probe_rtt_skipped_if_similar_rtt = false;
        bbr->config.probe_rtt_disabled_if_app_limited = false;

        /*初始化bbr->sampler*/
        bbr->sampler = sampler_create();
        //printf("[OOMDebug]::bbr_create\n");
        /*创建windows filter*/
        wnd_filter_init(&bbr->max_bandwidth, kBandwidthWindowSize, max_val_func);
        wnd_filter_init(&bbr->max_ack_height, kBandwidthWindowSize, max_val_func);
        bbr->default_bandwidth = kInitialBandwidthKbps;

        bbr->aggregation_epoch_start_time = -1;
        bbr->aggregation_epoch_bytes = 0;
        bbr->bytes_acked_since_queue_drained = 0;
        bbr->max_aggregation_bytes_multiplier = 0;
        bbr->min_rtt = 0;
        bbr->last_rtt = 0;
        bbr->last_PROBE_RTT_minrtt = 0;
        bbr->min_rtt_timestamp = 0;
        IsStart = true;
        /*初始化拥塞窗口*/
        bbr->congestion_window = bbr->config.initial_congestion_window;
        bbr->initial_congestion_window = bbr->config.initial_congestion_window;
        bbr->max_congestion_window = bbr->config.max_congestion_window;
        bbr->min_congestion_window = bbr->config.min_congestion_window;

        bbr->pacing_gain = 1;
        bbr->pacing_rate = 0;

        bbr->congestion_window_gain_constant = kProbeBWCongestionWindowGain;
        bbr->rtt_variance_weight = kBbrRttVariationWeight;
        bbr->cycle_current_offset = 0;
        bbr->last_cycle_start = 0;
        bbr->is_at_full_bandwidth = false;

        bbr->is_probe_bw_bandwidth_full = false;

        bbr->rounds_without_bandwidth_gain = 0;

        bbr->probe_bw_rounds_without_inflight_grow = 0;

        bbr->rounds_with_largelossrate = 0;
        bbr->bandwidth_at_last_round = 0;
        bbr->exiting_quiescence = false;
        bbr->exit_probe_rtt_at = -1;

        bbr->probe_rtt_round_passed = false;
        bbr->last_sample_is_app_limited = false;
        bbr->recovery_state = NOT_IN_RECOVERY;

        bbr->end_recovery_at = -1;
        bbr->recovery_window = bbr->max_congestion_window;
        bbr->app_limited_since_last_probe_rtt = false;
        bbr->min_rtt_since_last_probe_rtt = -1;

        bbr->constraints = *co;
        bbr->default_bandwidth = starting_bandwidth;

        //bbr_reset
        bbr->round_trip_count = 0;
        bbr->rounds_without_bandwidth_gain = 0;

        bbr->probe_bw_rounds_without_inflight_grow = 0;
        bbr->rounds_with_largelossrate = 0;
        if (bbr->config.num_startup_rtts > 0) {
            bbr->is_at_full_bandwidth = false;
            bbr->is_probe_bw_bandwidth_full = false;
            bbr_enter_startup_mode(bbr);
        } else {
            bbr->is_at_full_bandwidth = true;
            bbr->is_probe_bw_bandwidth_full = false;
            bbr_enter_probe_bandwidth_mode(bbr, bbr->constraints.at_time);
        }
        return bbr;
    }

    //销毁
    ////销毁反馈包处理器
    void SelfToRemoteQosUnit::bbr_feedback_adapter_destroy(bbr_fb_adapter_t *adapter) {
        if (adapter->hist != nullptr) {
            //销毁历史信息跳表
            sender_history_destroy(adapter->hist);
            adapter->hist = nullptr;
        }
        //销毁速率计算模块
        rate_stat_destroy(&adapter->acked_bitrate);
    }

    void SelfToRemoteQosUnit::sender_history_destroy(sender_history_t *hist) ////////////////////////////////待完善的地方
    {
        if (hist == nullptr)
            return;

        //printf("[OOMDebug]::sender_history_destroy, %d\n", hist);
        if (hist->l != nullptr) {
            //skiplist_destroy(hist->l);
            //解决了一个mismatch
            delete hist->l;
            hist->l = nullptr;
        }
        free(hist);
    }

    void SelfToRemoteQosUnit::rate_stat_destroy(rate_stat_t *rate) {
        if (rate == NULL)
            return;

        if (rate->buckets != NULL) {
            free(rate->buckets);
            rate->buckets = NULL;
        }
    }

    void SelfToRemoteQosUnit::bbr_destroy(bbr_controller_t *bbr) {
        if (bbr == nullptr)
            return;

        if (bbr->sampler != nullptr) {
            sampler_destroy(bbr->sampler);
            bbr->sampler = nullptr;
        }

        free(bbr);
        //printf("[OOMDebug]::bbr_destroy\n");
    }

    void SelfToRemoteQosUnit::sampler_destroy(bbr_bandwidth_sampler_t *sampler) {
        if (sampler != nullptr) {
            //printf("[OOMDebug]::Sampler_destroy , %d\n", sampler);
            delete sampler->points;
            free(sampler);
        }
    }


    //sampler的创建
    //TODO:Leak_definitelyLost
    bbr_bandwidth_sampler_t *SelfToRemoteQosUnit::sampler_create() {
        auto *sampler = (bbr_bandwidth_sampler_t *) calloc(1, sizeof(bbr_bandwidth_sampler_t));
        sampler->size = kDefaultPoints;
        //sampler->points = (bbr_packet_point_t*)malloc(sampler->size * sizeof(bbr_packet_point_t));
        sampler->points = new SendCircularDataQueue();
        sampler_reset(sampler);
        //printf("[OOMDebug]::struct bbr_bandwidth_sampler_t Calloc : %d\n", sampler);
        return sampler;
    }

    void SelfToRemoteQosUnit::sampler_reset(bbr_bandwidth_sampler_t *sampler) {
        int i;
        bbr_packet_point_t *point;

        sampler->total_data_sent = 0;
        sampler->total_data_acked = 0;
        sampler->total_data_sent_at_last_acked_packet = 0;
        sampler->last_acked_packet_sent_time = -1;
        sampler->last_acked_packet_ack_time = -1;
        sampler->last_sent_packet = 0;
        sampler->rate_bps = -1;
        sampler->roundtrip = -1;////0728改
        sampler->is_app_limited = 0;
        sampler->is_recv_burst = 0;
        sampler->last_acked_packet_app_limited = 0;
        sampler->end_of_app_limited_phase = 0;

        sampler->start_pos = -1;
        sampler->index = -1;
        sampler->count = 0;
        /*for (i = 0; i < sampler->size; ++i) {
            point = &sampler->points[i];

            point->send_time = 0;
            point->size = 0;

            point->total_data_sent = 0;
            point->total_data_acked_at_the_last_recv_fbpacket = 0;
            point->total_data_sent_at_last_acked_packet = 0;

            point->last_acked_packet_ack_time = -1;
            point->last_acked_packet_sent_time = -1;
            point->is_app_limited = 0;
            point->ignore = 1;
        }*/
    }

    void SelfToRemoteQosUnit::sender_history_add(sender_history_t *hist, packet_feedback *packet) {
        packet_feedback *p;
        shared_ptr<packet_feedback> ptemp;
        /*skiplist_iter_t* it;
        skiplist_item_t key, val;*/
        /*去除过期的发送记录*/
        /*while (skiplist_size(hist->l) > 0)
        {
            it = skiplist_first(hist->l);
            p = (packet_feedback*)it->val.ptr;
            if (p->create_ts + hist->limited_ms < packet->send_ts)
            {
                if (it->key.i64 > hist->last_ack_seq_num)
                {
                    hist->outstanding_bytes = SU_MAX(hist->outstanding_bytes - p->payload_size, 0);
                    std::cout << "number:" << p->sequence_number;
                    std::cout << "outstanding_bytes:" << hist->outstanding_bytes << std::endl;
                    hist->last_ack_seq_num = it->key.i64;
                }
                skiplist_remove(hist->l, it->key);
            }
            else
                break;
        }*/
        /*p = (packet_feedback*)malloc(sizeof(packet_feedback));
        *p = *packet;*/
        ptemp = make_shared<packet_feedback>(*packet);
        //key.i64 = packet->sequence_number+ hist->sendpacketroundtrip*PacketSeqloopSize;
        //val.ptr = p;
        //skiplist_insert(hist->l, key, val);
        hist->l->Qos_DealSingleSendData(ptemp);
        hist->outstanding_bytes += packet->payload_size;
        if (hist->last_ack_seq_num == 0) {
            hist->last_ack_seq_sendtime = 0;///////////////////////存疑，如果某一次ack序号正好是0，可能会出现问题
        }
    }

    void SelfToRemoteQosUnit::sampler_on_packet_sent(bbr_bandwidth_sampler_t *sampler, int64_t sent_time,
                                                     int64_t packet_number, size_t data_size, size_t data_in_flight) {
        sampler->last_sent_packet = packet_number;
        sampler->total_data_sent += data_size;
        if (sampler->total_data_acked <= 0) {
            sampler->last_acked_packet_ack_time = sent_time;
            sampler->last_acked_packet_sent_time = sent_time;
            sampler->total_data_sent_at_last_acked_packet = sampler->total_data_sent;
            datatotlesendlastack_acktime = sent_time;
        }
        sampler_add_point(sampler, sent_time, packet_number, data_size);
    }

    void SelfToRemoteQosUnit::sampler_add_point(bbr_bandwidth_sampler_t *sampler, int64_t sent_time, int64_t number,
                                                size_t data_size) {
        //bbr_packet_point_t* point;
        shared_ptr<bbr_packet_point_t> point;
        int pos = 0;;
        /*if (number <= sampler->start_pos)
            return;*/  ///////////////////////////////////加上这个和减去这个有什么区别
        if (sampler->start_pos == -1) {
            sampler->start_pos = number;
        }
        sampler->index = number;
        pos = number;
        //pos = sampler->points->QosSend_SetPosition(number);
        point = sampler->points->QosSend_GetItemByIndex(pos);
        point->size = data_size;
        point->send_time = sent_time;
        if (ack_message_before_n_feedback.size() >= 10) {
            shared_ptr<feedback_ack_message> fbmsg = ack_message_before_n_feedback.front();
            point->total_data_acked_at_the_last_acked_packet_before_n_feedback = fbmsg->total_data_acked;
            point->last_acked_packet_before_n_feedback_ack_time = fbmsg->last_acked_packet_ack_time;
        } else {
            point->total_data_acked_at_the_last_acked_packet_before_n_feedback = 0;
            point->last_acked_packet_before_n_feedback_ack_time = sent_time;
        }
        sampler_to_point(sampler, point);
        sampler->count++;
    }

    inline void
    SelfToRemoteQosUnit::sampler_to_point(bbr_bandwidth_sampler_t *sampler, shared_ptr<bbr_packet_point_t> point) {
        //std::cout << "jinru"  << std::endl;
        point->total_data_sent = sampler->total_data_sent;
        point->total_data_acked_at_the_last_recv_fbpacket = sampler->total_data_acked;
        point->total_data_sent_at_last_acked_packet = sampler->total_data_sent_at_last_acked_packet;
        point->last_acked_packet_is_app_limited = sampler->last_acked_packet_app_limited;
        point->last_acked_packet_ack_time = sampler->last_acked_packet_ack_time;
        point->last_acked_packet_sent_time = sampler->last_acked_packet_sent_time;


        point->is_app_limited = sampler->is_app_limited;
        point->is_recv_burst = sampler->is_recv_burst;

        point->ignore = 0;
    }

    /*void SelfToRemoteQosUnit::sampler_resize_points(bbr_bandwidth_sampler_t* sampler, int size)
    {
        int64_t i;
        int old_pos, new_pos, new_size;
        bbr_packet_point_t* new_points;

        if (size <= sampler->size)
            return;

        new_size = sampler->size;
        while (size > new_size)
            new_size *= 2;
        new_points = (bbr_packet_point_t*)malloc(new_size * sizeof(bbr_packet_point_t));
        for (i = sampler->start_pos; i < sampler->index; ++i) {
            old_pos = i % sampler->size;
            new_pos = i % new_size;
            new_points[new_pos] = sampler->points[old_pos];
        }

        free(sampler->points);
        sampler->points = new_points;
        sampler->size = new_size;
    }*/
    size_t SelfToRemoteQosUnit::sampler_total_data_acked(bbr_bandwidth_sampler_t *sampler) {
        return sampler->total_data_acked;
    }
    //void SelfToRemoteQosUnit::sampler_remove_point(bbr_bandwidth_sampler_t* sampler, int64_t number, int packetseqroundtrip)
    //{
    //	bbr_packet_point_t* point;
    //	int pos;

    //	if (sampler->start_pos > number || sampler->index < number)
    //		return;
    //	//std::cout << "packetseqroundtrip"<< packetseqroundtrip << std::endl;
    //	pos = number % sampler->size;
    //	//std::cout << "pos" << pos << std::endl;
    //	point = &sampler->points[pos];
    //	//std::cout << "data xiaochu:" << pos  << std::endl;
    //	point->ignore = 1;

    //	/*进行数据抹除*/
    //	//std::cout << "data xiaochu:" << sampler->start_pos << "number:" << number <<"position:"<<pos<< std::endl;
    //	if (sampler->start_pos == number) {
    //		point->send_time = 0;
    //		point->size = 0;
    //		point->total_data_sent = 0;
    //		point->total_data_acked_at_the_last_recv_fbpacket = 0;
    //		point->total_data_sent_at_last_acked_packet = 0;
    //		point->last_acked_packet_ack_time = -1;
    //		point->last_acked_packet_sent_time = -1;
    //		point->is_app_limited = 0;

    //		if (sampler->index > sampler->start_pos)
    //		{
    //			sampler->start_pos++;
    //			//std::cout << "sampler->start_pos增加了" << sampler->start_pos << std::endl;
    //		}
    //	}
    //	//std::cout << "sampler->start_pos" << sampler->start_pos << std::endl;
    //	if (sampler->count > 0)
    //		sampler->count--;
    //}
    bbr_bandwidth_sample_t
    SelfToRemoteQosUnit::sampler_on_packet_acked(bbr_bandwidth_sampler_t *sampler, int64_t ack_time,
                                                 int64_t fb_send_time, shared_ptr<bbr_packet_point_t> point,
                                                 uint32_t acked_bitrate) {
        bbr_bandwidth_sample_t ret = {0};
        ret = sampler_on_packet_acked_inner(sampler, ack_time, fb_send_time, point, acked_bitrate);
        //sampler_remove_point(sampler, number);
        return ret;
    }

    DWORD changepointdetect_start = 0;

    bbr_bandwidth_sample_t
    SelfToRemoteQosUnit::sampler_on_packet_acked_inner(bbr_bandwidth_sampler_t *sampler, int64_t ack_time,
                                                       int64_t fb_send_time, shared_ptr<bbr_packet_point_t> point,
                                                       uint32_t acked_bitrate) {
        int32_t send_rate, ack_rate, ackrate_before_n_feedback;
        int delta_ack_time, delta_ack_time_before_n_feedback;
        int32_t middle_rate;

        bbr_bandwidth_sample_t ret = {0};
        if (point->total_data_acked_at_the_last_recv_fbpacket != 0 &&
            (fb_send_time - sampler->last_acked_packet_ack_time) > 100) {
            is_recv_block = true;
            recv_block_stop_sampling_num = 0;
            recv_block_stop_time = sampler->total_data_acked;
        }
        sampler->total_data_acked += feedback.feedback.data_acked;
        sampler->total_data_sent_at_last_acked_packet = point->total_data_sent;
        sampler->last_acked_packet_ack_time = fb_send_time;
        sampler->last_acked_packet_sent_time = point->send_time;
        sampler->last_acked_packet_app_limited = point->is_app_limited;

        if (is_recv_block) {
            if (point->total_data_acked_at_the_last_recv_fbpacket > recv_block_stop_time) {
                recv_block_stop_sampling_num++;
                if (recv_block_stop_sampling_num > krRecv_Block_Stop_Sampling_num) {
                    is_recv_block = false;
                }
            }

        }
        if (feedback_packet_count <= kFeedBacknumber) {
            feedback_packet_count++;
            shared_ptr<feedback_ack_message> msg = make_shared<feedback_ack_message>();
            msg->last_acked_packet_ack_time = fb_send_time;
            msg->total_data_acked = sampler->total_data_acked;
            ack_message_before_n_feedback.push(msg);
        } else {
            ack_message_before_n_feedback.pop();
            shared_ptr<feedback_ack_message> msg = make_shared<feedback_ack_message>();
            msg->last_acked_packet_ack_time = fb_send_time;
            msg->total_data_acked = sampler->total_data_acked;
            ack_message_before_n_feedback.push(msg);
        }
        //if (feedback.feedback.data_acked==0 && number!=sampler->last_sent_packet) {
        //	sampler->is_recv_burst = 1; //当某一次反馈端10ms确认数据量为正常确认数据量的1/2,说明此事接收端处于突发接收阶段，应该过滤此时采集到的带宽值。
        //}
        //else {
        //	sampler->is_recv_burst = 0;
        //}
        if (sampler->is_app_limited == 1 &&
            /* number > sampler->end_of_app_limited_phase&&*/point->is_app_limited == 1) {
            sampler->is_app_limited = 0;
        }

        if (point->last_acked_packet_ack_time == -1 || point->last_acked_packet_sent_time == -1) {
            return ret;
        }
        /*计算send_rate*/
        send_rate = 0x7fffffff;
        if (point->send_time > point->last_acked_packet_sent_time) {
//            printf("[QOSDebug]::Send_Rate : point->total_data_sent:%zu\n",point->total_data_sent);
//            printf("[QOSDebug]::Send_Rate : point->total_data_sent_at_last_acked_packet:%zu\n",point->total_data_sent_at_last_acked_packet);
//            printf("[QOSDebug]::Send_Rate : point->send_time :%zu\n",point->send_time);
//            printf("[QOSDebug]::Send_Rate : point->last_acked_packet_sent_time :%ld\n\n",point->last_acked_packet_sent_time);
            send_rate = (point->total_data_sent - point->total_data_sent_at_last_acked_packet) /
                        ((int) (point->send_time - point->last_acked_packet_sent_time));
        } else {
//            printf("[QOSDebug]::Send_Rate : point->total_data_sent:%zu\n",point->total_data_sent);
//            printf("[QOSDebug]::Send_Rate : point->total_data_sent_at_last_acked_packet:%zu\n",point->total_data_sent_at_last_acked_packet);
//            printf("[QOSDebug]::Send_Rate : point->send_time :%zu\n",point->send_time);
//            printf("[QOSDebug]::Send_Rate : point->last_acked_packet_sent_time :%ld\n\n",point->last_acked_packet_sent_time);
        }
        //if (fb_send_time > point->last_acked_packet_ack_time + 2) {
        if (point->total_data_acked_at_the_last_recv_fbpacket == 0) {
            delta_ack_time = GET_SYS_TIME() - point->last_acked_packet_ack_time;
        } else {
            delta_ack_time = fb_send_time - point->last_acked_packet_ack_time;
        }
        if (point->total_data_acked_at_the_last_acked_packet_before_n_feedback == 0) {
            //printf("timestamp:%ld - last_acked_packet_before_n_feedback_ack_time:%ld \n",GET_SYS_TIME(),point->last_acked_packet_before_n_feedback_ack_time);
            delta_ack_time_before_n_feedback = GET_SYS_TIME() - point->last_acked_packet_before_n_feedback_ack_time;
        } else {
            //printf("fb_send_time:%ld - last_acked_packet_before_n_feedback_ack_time:%ld \n",fb_send_time,point->last_acked_packet_before_n_feedback_ack_time);
            delta_ack_time_before_n_feedback = fb_send_time - point->last_acked_packet_before_n_feedback_ack_time;
        }

        ack_rate = (sampler->total_data_acked - point->total_data_acked_at_the_last_recv_fbpacket) /
                   (int) (delta_ack_time + 1);
        //printf("[QOSDebug]::ack_rate : ack_rate:%d\n",ack_rate);
        ackrate_before_n_feedback =
                (sampler->total_data_acked - point->total_data_acked_at_the_last_acked_packet_before_n_feedback) /
                (int) (delta_ack_time_before_n_feedback + 1);
//        printf("[QOSDebug]::ack_rate : ackrate_before_n_feedback:%d\n",ackrate_before_n_feedback);
//        printf("[QOSDebug]::ack_rate : sampler->total_data_acked:%zu\n",sampler->total_data_acked);
//        printf("[QOSDebug]::ack_rate : point->total_data_acked_at_the_last_acked_packet:%zu\n",point->total_data_acked_at_the_last_acked_packet_before_n_feedback);
//        printf("[QOSDebug]::ack_rate : delta_ack_time_before_n_feedback:%d\n\n",(int) (delta_ack_time_before_n_feedback + 1));
        //ret.bandwidth = SU_MIN(ack_rate, send_rate);
        //ret.bandwidth = SU_MIN(ack_rate,acked_bitrate);///采集到带宽样本值为ackrate与recvrate最小值
        //ret.bandwidth = SU_MIN(ack_rate, acked_bitrate);
        if (bbr_bandwidth_estimate(bbr) < 125) {
            ret.bandwidth = SU_MIN(ackrate_before_n_feedback, ack_rate);
        } else {
            //middle_rate = SU_MIN(ack_rate, acked_bitrate);
            if (point->total_data_acked_at_the_last_acked_packet_before_n_feedback == 0) {
                ret.bandwidth = SU_MIN(ack_rate, ackrate_before_n_feedback);
            } else {
                ret.bandwidth = ackrate_before_n_feedback;
            }
            //ret.bandwidth = SU_MIN(ack_rate, send_rate);

        }
        /*if (ret.bandwidth < 580) {
            ret.bandwidth = 580;
        }*/
        /*if (ret.bandwidth > 12500) {
            ret.bandwidth = 12500;
        }
        if (ret.bandwidth < 125) {
            ret.bandwidth = 125;
        }*/

//        printf("[QOSDebug]::Rtt calculate : ack_time is %ld\n",ack_time);
//        printf("[QOSDebug]::Rtt calculate : point->send_time is %ld\n\n",point->send_time);
        ret.rtt = ack_time - point->send_time;

        if (nullptr != g_qos_rtt_callback) {
            g_qos_rtt_callback(ret.rtt);
        }
        ret.is_app_limited =
                point->last_acked_packet_is_app_limited || point->is_app_limited;///***************************重要
        ret.is_recv_burst = point->is_recv_burst;
        if (ret.bandwidth < 500) { is_recv_block = false; } //若此时链路带宽为500kBps（4Mbps）时，相邻反馈包间隔大于100ms属于正常情况，，不应该将采集带宽样本忽略
        return ret;
    }

    void SelfToRemoteQosUnit::sampler_on_app_limited(bbr_bandwidth_sampler_t *sampler) {
        sampler->is_app_limited = 1;
        sampler->end_of_app_limited_phase = sampler->last_sent_packet;
    }

    void SelfToRemoteQosUnit::sampler_remove_old(bbr_bandwidth_sampler_t *sampler, int64_t least_unacked,
                                                 int packetseqroundtrip) {
        /*	while (sampler->start_pos < least_unacked + PacketSeqloopSize * packetseqroundtrip
                && sampler->start_pos < sampler->index)
            {
                sampler_remove_point(sampler, sampler->start_pos, packetseqroundtrip);
            }*/
        sampler->points->QosSend_SetHeadIndexStepByStep(least_unacked);
        sampler->start_pos = sampler->points->QosSend_GetHeadIndex();
        sampler->count = sampler->points->m_sendpacketsqueue_size;
    }


    //带宽估计
    void SelfToRemoteQosUnit::bbr_on_send_packet(bbr_controller_t *bbr, bbr_packet_info_t *packet) {
        bbr->last_sent_packet = packet->seq;
        if (packet->data_in_flight == 0 && bbr->sampler->is_app_limited == 1)
            bbr->exiting_quiescence = true;

        if (bbr->aggregation_epoch_start_time == -1)
            bbr->aggregation_epoch_start_time = packet->send_time;

        /*记录发送的报序列信息*/
        //□□□□□□□□□□□□□□□□□□□□□□
        //1 / 2 / 3 / 4....填充
        //	若反馈包中确认最后一个包序号为50，
        //	startpos = 50；
        //	若startpos + size <= number
        //	pos = number / size
        //	若startpos + size > number
        //	newsize = 2 * size
        //	pos = number / size
        sampler_on_packet_sent(bbr->sampler, packet->send_time, packet->seq, packet->size, packet->data_in_flight);
        //return bbr_create_rate_update(bbr, packet->send_time);
    }

    bbr_network_ctrl_update_t SelfToRemoteQosUnit::bbr_create_rate_update(bbr_controller_t *bbr, int64_t at_time) {
        int32_t bandwidth, target_rate, pacing_rate;
        int64_t rtt;
        bbr_network_ctrl_update_t ret;

        ret.congestion_window = -1;
        if (at_time == -1)
            return ret;

        /*wnd_filter_print(&bbr->max_bandwidth);*/
        rtt = bbr->rtt_stat.smoothed_rtt;

        /*返回拥塞控制窗口*/
        ret.congestion_window = bbr_get_congestion_window(bbr);
        if (rtt <= 0)
            bandwidth = bbr->default_bandwidth;
        else
            bandwidth = ret.congestion_window / rtt;

        /*确定pacing rate和target rate*/
        pacing_rate = bbr_pacing_rate(bbr);
        target_rate = bbr->config.pacing_rate_as_target ? pacing_rate : bandwidth;
        if (bbr->mode == PROBE_RTT)
            target_rate = (int32_t) (target_rate * bbr->config.encoder_rate_gain_in_probe_rtt);
        else {
            target_rate = (int32_t) (target_rate * bbr->config.encoder_rate_gain);
        }

        if (bbr->constraints.at_time > 0) {
            if (bbr->constraints.max_rate > 0) {
                target_rate = SU_MIN(target_rate, bbr->constraints.max_rate);
                pacing_rate = SU_MIN(pacing_rate, bbr->constraints.max_rate);
            }
            if (bbr->constraints.min_rate > 0) {
                target_rate = SU_MAX(target_rate, bbr->constraints.min_rate);
                pacing_rate = SU_MAX(pacing_rate, bbr->constraints.min_rate);
            }
        }

        /*返回target_rate信息*/
        ret.target_rate.at_time = at_time;
        ret.target_rate.bandwidth = bandwidth;
        ret.target_rate.rtt = SU_MAX(rtt, 8);

        ret.target_rate.loss_rate_ratio = bbr_loss_filter_get(&bbr->loss_rate);
        ret.target_rate.bwe_period = rtt * kGainCycleLength;
        //std::cout << "target_rate00004" << target_rate << std::endl;
        ret.target_rate.target_rate = target_rate;

        /*返回pacer信息*/
        ret.pacer_config.at_time = at_time;
        ret.pacer_config.time_window = rtt > 20 ? (rtt / 4) : 5;
        ret.pacer_config.data_window = (size_t) (ret.pacer_config.time_window * pacing_rate);
        if (bbr_is_probing_for_more_bandwidth(bbr) == 1)
            ret.pacer_config.pad_window = ret.pacer_config.time_window * target_rate;
        else
            ret.pacer_config.pad_window = 0;

        return ret;
    }

    int32_t SelfToRemoteQosUnit::bbr_bandwidth_estimate(bbr_controller_t *bbr) {
        return ((int32_t) wnd_filter_best(&bbr->max_bandwidth));
    }

    void SelfToRemoteQosUnit::rate_stat_update(rate_stat_t *rate, size_t count, int64_t now_ts) {
        int ts_offset, index;

        if (rate->oldest_ts > now_ts)
            return;

        rate_stat_erase(rate, now_ts);

        if (rate->oldest_ts == -1) {
            rate->oldest_ts = now_ts;
        }

        ts_offset = (int) (now_ts - rate->oldest_ts);
        index = (rate->oldest_index + ts_offset) % rate->wnd_size;

        rate->buckets[index].sum += count;
        rate->buckets[index].sample++;

        rate->sample_num++;
        rate->accumulated_count += count;
    }

    void SelfToRemoteQosUnit::rate_stat_erase(rate_stat_t *rate, int64_t now_ts) {
        int64_t new_oldest_ts;
        rate_bucket_t *bucket;

        if (rate->oldest_ts == -1)
            return;

        new_oldest_ts = (int) (now_ts - rate->wnd_size + 1);   //for test
        if (new_oldest_ts <= rate->oldest_ts)
            return;

        while (rate->sample_num > 0 && rate->oldest_ts < new_oldest_ts) {
            bucket = &rate->buckets[rate->oldest_index];

            rate->sample_num -= bucket->sample;
            rate->accumulated_count -= bucket->sum;
            bucket->sum = 0;
            bucket->sample = 0;

            if (++rate->oldest_index >= rate->wnd_size)
                rate->oldest_index = 0;

            ++rate->oldest_ts;
        }

        rate->oldest_ts = new_oldest_ts;
    }

    int SelfToRemoteQosUnit::rate_stat_rate(rate_stat_t *rate, int64_t now_ts) {
        int ret, active_wnd_size;

        rate_stat_erase(rate, now_ts);

        active_wnd_size = (int) (now_ts - rate->oldest_ts + 1);
        if (rate->sample_num == 0 || active_wnd_size <= 1 || (active_wnd_size < rate->wnd_size))
            return -1;

        ret = (int) (rate->accumulated_count * rate->scale / active_wnd_size + 0.5);

        return ret;
    }

    int32_t SelfToRemoteQosUnit::bbr_feedback_get_bitrate(bbr_fb_adapter_t *adapter) {
        return rate_stat_rate(&adapter->acked_bitrate, adapter->feedback.feedback_time);
    }


    //拥塞窗口
    int32_t SelfToRemoteQosUnit::bbr_get_congestion_window(bbr_controller_t *bbr) {
        /*如果是评估最小rtt模式下，需要取最小的并发窗口*/
        if (bbr->mode == PROBE_RTT)
            return bbr_probe_rtt_congestion_window(bbr);

        /*如果是在recovery阶段且受recover window的约束，返回约束后的值*/
        if (bbr_in_recovery(bbr) && !bbr->config.rate_based_recovery
            && !(bbr->mode == STARTUP && bbr->config.rate_based_startup))
            return SU_MIN(bbr->congestion_window, bbr->recovery_window);

        return bbr->congestion_window;
    }

    /*获取在probe_rtt模式下的拥塞窗口大小*/
    size_t SelfToRemoteQosUnit::bbr_probe_rtt_congestion_window(bbr_controller_t *bbr) {
        if (bbr->config.probe_rtt_based_on_bdp) {
            return bbr_get_target_congestion_window(bbr, /*bbr->config.probe_rtt_congestion_window_gain*/
                                                    bbr->config.real_probe_rtt_congestion_window_gain);
        }//bbr->config.probe_rtt_congestion_window_gain=0.75
        else
            return bbr->min_congestion_window;
    }

    /*计算BDP的发送窗大小*/
    size_t SelfToRemoteQosUnit::bbr_get_target_congestion_window(bbr_controller_t *bbr, double gain) {
        size_t bdp, congestion_window;
        bdp = (int32_t) (bbr_get_min_rtt(bbr) * bbr_bandwidth_estimate(bbr));
        congestion_window = (size_t) (gain * bdp);

        if (congestion_window <= 0)
            congestion_window = (size_t) (gain * bbr->initial_congestion_window);

        if (congestion_window <
            bbr->min_congestion_window);/* razor_debug("bdp = %d, max bandwidth = %u, min rtt = %u\n", congestion_window, bbr_bandwidth_estimate(bbr), bbr_get_min_rtt(bbr));*/

        return SU_MAX(congestion_window, bbr->min_congestion_window);
    }

    //pacing_rate
    int32_t SelfToRemoteQosUnit::bbr_pacing_rate(bbr_controller_t *bbr) {
        if (bbr->pacing_rate == 0)
            return (int32_t) (kHighGain * bbr->initial_congestion_window / bbr_get_min_rtt(bbr));
        else
            return bbr->pacing_rate;
    }

    double SelfToRemoteQosUnit::bbr_get_pacing_gain(bbr_controller_t *bbr, int index) {
        if (index == 0)
            return 1 + bbr->config.probe_bw_pacing_gain_offset;
        else if (index == 1)
            return 1 - bbr->config.probe_bw_pacing_gain_offset;
        else
            return 1;
    }

    //带宽滤波器
    void SelfToRemoteQosUnit::wnd_filter_init(windowed_filter_t *filter, int64_t wnd_size, compare_func_f comp) {
        filter->wnd_size = wnd_size;
        filter->comp_func = comp;

        wnd_filter_reset(filter, 0, 0);
    }

    void SelfToRemoteQosUnit::wnd_filter_reset(windowed_filter_t *filter, int64_t new_sample, int64_t new_ts) {
        int i;
        for (i = 0; i < 3; i++) {
            filter->estimates[i].sample = new_sample;
            filter->estimates[i].ts = new_ts;
        }
    }

    int64_t SelfToRemoteQosUnit::wnd_filter_best(windowed_filter_t *filter) {
        return filter->estimates[0].sample;
    }

    void SelfToRemoteQosUnit::wnd_filter_update(windowed_filter_t *filter, int64_t new_sample, int64_t new_ts) {
        wnd_sample_t sample = {new_sample, new_ts};

        /*如果这是第一个更新、filter中记录的值都过了时间窗或者是最大的值，用新的值进行覆盖更新*/
        if (filter->estimates[0].sample == 0 || filter->comp_func(new_sample, filter->estimates[0].sample) > 0
            || new_ts - filter->estimates[2].ts > filter->wnd_size) {
            wnd_filter_reset(filter, new_sample, new_ts);
            return;
        }

        /*新更新的值是第二大的值，对1，2位的值进行更新*/
        if (filter->comp_func(new_sample, filter->estimates[1].sample) > 0) {
            filter->estimates[1] = sample;
            filter->estimates[2] = sample;
        } else if (filter->comp_func(new_sample, filter->estimates[2].sample) > 0) {/*更新的值为第三大，只对末尾的2进行更新*/
            filter->estimates[2] = sample;
        }

        /*进行过期淘汰,先判断0位是否淘汰，再判断1是否淘汰,因为2位上在第一个if上做了判断*/
        if (new_ts - filter->estimates[0].ts > filter->wnd_size) {
            filter->estimates[0] = filter->estimates[1];
            filter->estimates[1] = filter->estimates[2];
            filter->estimates[2] = sample;

            /*再次对移动后的filter进行过期淘汰判断*/
            if (new_ts - filter->estimates[0].ts > filter->wnd_size) {
                filter->estimates[0] = filter->estimates[1];
                filter->estimates[1] = filter->estimates[2];
            }

            return;
        }

        /*按时间戳先后进行相同值的更新*/
        if (filter->estimates[0].sample == filter->estimates[1].sample
            && new_ts - filter->estimates[1].ts >
               (filter->wnd_size >> 2)) {/*0号位上最大的值和1号位上的值相等，但1号位的时间戳点距离当前时间点超过窗口的1/4,意味着1 2号位的需要更新到最新值上来*/
            filter->estimates[1] = filter->estimates[2] = sample;
        }

        /*同上原理，对2号位上的值进行更新*/
        if (filter->estimates[1].sample == filter->estimates[2].sample
            && new_ts - filter->estimates[2].ts > (filter->wnd_size >> 1))
            filter->estimates[2] = sample;
    }

    int64_t SelfToRemoteQosUnit::wnd_filter_third_best(windowed_filter_t *filter) {
        return filter->estimates[2].sample;
    }

    //往返时延
    int64_t SelfToRemoteQosUnit::bbr_initial_rtt_us(bbr_rtt_stat_t *s) {
        return s->initial_rtt_us;
    }

    int64_t SelfToRemoteQosUnit::bbr_get_min_rtt(bbr_controller_t *bbr) {

        if (bbr->min_rtt == 0)
            return bbr_initial_rtt_us(&bbr->rtt_stat) / 1000;
        else
            return bbr->min_rtt;
    }

    void SelfToRemoteQosUnit::bbr_rtt_update(bbr_rtt_stat_t *s, int64_t send_delta, int64_t ack_delay, int64_t now_ts) {
        int64_t rtt_sample;
        if (send_delta <= 0) {
            return;
        }

        /*if (s->min_rtt == 0 || s->min_rtt > send_delta)
            s->min_rtt = SU_MAX(send_delta, 5);*/


        rtt_sample = SU_MAX(5, send_delta);
        s->previous_srtt = s->smoothed_rtt;
        if (rtt_sample > ack_delay) {
            rtt_sample = rtt_sample - ack_delay;
        }

        s->latest_rtt = rtt_sample;

        if (s->smoothed_rtt == 0) {
            s->smoothed_rtt = rtt_sample;
            s->mean_deviation = rtt_sample / 2;
        } else {
            s->mean_deviation = kOneMinusBeta * s->mean_deviation + kBeta * SU_ABS(s->smoothed_rtt, s->latest_rtt);
            //std::cout << "s->smoothed_rtt" << s->smoothed_rtt << "rtt_sample" << rtt_sample << std::endl;
            s->smoothed_rtt = kOneMinusAlpha * s->smoothed_rtt + kAlpha * rtt_sample;
            //std::cout << "s->smoothed_rtt:" << s->smoothed_rtt << std::endl;
        }
    }


    //BBR状态转化
    inline int SelfToRemoteQosUnit::bbr_in_recovery(bbr_controller_t *bbr) {
        return bbr->recovery_state != NOT_IN_RECOVERY ? true : false;
    }

    /*bbr进入STARTUP模式*/
    void SelfToRemoteQosUnit::bbr_enter_startup_mode(bbr_controller_t *bbr) {
        bbr->mode = STARTUP;
        bbr->pacing_gain = kHighGain;
        bbr->congestion_window_gain = kHighGain;
    }

    /*bbr进入带宽评估模式*/
    void SelfToRemoteQosUnit::bbr_enter_probe_bandwidth_mode(bbr_controller_t *bbr, int64_t now_ts) {
        bbr->mode = PROBE_BW;
        bbr->congestion_window_gain = bbr->congestion_window_gain_constant;

        //bbr->cycle_current_offset = rand() % (kGainCycleLength - 1);
        bbr->cycle_current_offset = 0;
        if (bbr->cycle_current_offset >= 1)
            bbr->cycle_current_offset += 1;

        bbr->last_cycle_start = now_ts;
        bbr->pacing_gain = bbr_get_pacing_gain(bbr, bbr->cycle_current_offset);
    }

    int SelfToRemoteQosUnit::bbr_is_probing_for_more_bandwidth(bbr_controller_t *bbr) {
        return ((bbr->mode == PROBE_BW && bbr->pacing_gain > 1) || bbr->mode == STARTUP) ? true : false;
    }

    //DWORD changepointdetect_start = 0;
    int
    SelfToRemoteQosUnit::bbr_update_bandwidth_and_min_rtt(bbr_controller_t *bbr, int64_t now_ts, int64_t fb_send_time,
                                                          shared_ptr<bbr_packet_point_t> point, uint32_t bandwidth) {
        int i, min_rtt_expired;
        int64_t sample_rtt = -1;
        is_bandwidth_full = false;
        bbr_bandwidth_sample_t sample;
        //for (i = 0; i < size; ++i) {
        sample = sampler_on_packet_acked(bbr->sampler, now_ts, fb_send_time, point, bandwidth);
        bbr->last_sample_is_app_limited = sample.is_app_limited;

        /*对RTT的计算*/
        if (sample.rtt > 0) {
            if (sample_rtt == -1)
                sample_rtt = sample.rtt;
            else
                sample_rtt = SU_MIN(sample_rtt, sample.rtt);
        }
        //}
        //std::cout << "sample.rtt" << sample.rtt << std::endl;
        if (sample_rtt == -1)
            //return false;
            bbr_rtt_update(&bbr->rtt_stat, sample_rtt, 0, now_ts);
        /*进行带宽统计和滤波*/
        if (!is_recv_block && (!sample.is_app_limited ||
                               sample.bandwidth > bbr_bandwidth_estimate(bbr)))///***********更改为0.9倍就纳入到带宽过滤器中，采取其带宽的样本值
        {
            if (!sample.is_recv_burst) {
                wnd_filter_update(&bbr->max_bandwidth, sample.bandwidth,
                                  bbr->round_trip_count); /*bandwidth <= bbr->constraints.min_rate ? sample.bandwidth : bandwidth*/
            }

        }
        bbr->last_rtt = sample_rtt;
        if (bbr->min_rtt_since_last_probe_rtt == -1)
            bbr->min_rtt_since_last_probe_rtt = sample_rtt;
        else
            bbr->min_rtt_since_last_probe_rtt = SU_MIN(bbr->min_rtt_since_last_probe_rtt, sample_rtt);
        //std::cout << "sample rtt" << sample_rtt << std::endl;
        /*评估是否要RTT的作用时间是否过期*/
        min_rtt_expired = (bbr->min_rtt > 0 && now_ts > (bbr->min_rtt_timestamp + kMinRttExpiry)) ? true : false;
        //std::cout << "min_rtt_expired:" << min_rtt_expired << std::endl;
        //std::cout << "min_rtt_expired:" << min_rtt_expired << std::endl;
        is_bandwidth_full = (sample_rtt > bbr->min_rtt * kRTT_congestion_threshold) ? true : false;
        if (min_rtt_expired || sample_rtt < bbr->min_rtt || bbr->min_rtt <= 0) {
            /*if (bbr_should_extend_min_rtt_expiry(bbr))
                min_rtt_expired = false;
            else*/
            //if (sample_rtt < bbr->min_rtt || bbr->min_rtt <= 0) {
            //	bbr->min_rtt = SU_MAX(5, sample_rtt);
            //}////////////更改问题****************************
            if (min_rtt_expired) {
                bbr->last_PROBE_RTT_minrtt = bbr->min_rtt;//记录上一阶段采集到的最小rtt值
            }
            if (sample_rtt > bbr->last_PROBE_RTT_minrtt && (min_rtt_expired || bbr->mode ==
                                                                               PROBE_RTT)) {//当刚进入RTT状态或者处于RTT状态时，并且此时采集到的minrtt过大于之前采集的最小minrtt时，调整计算probe_rtt Bdp值的增益。
                //防止进入到probe rtt状态时，即刻更新的minrtt过大，造成计算的BDP立即大于inflight，probe rtt来不及进行减速
                bbr->config.real_probe_rtt_congestion_window_gain =
                        bbr->config.probe_rtt_congestion_window_gain * bbr->last_PROBE_RTT_minrtt / sample_rtt;
            }
            if (sample_rtt < 1.25 * bbr->last_PROBE_RTT_minrtt && (min_rtt_expired || bbr->mode == PROBE_RTT)) {
                bbr->config.real_probe_rtt_congestion_window_gain = 0.75;
            }
            bbr->min_rtt = SU_MAX(5, sample_rtt);
            bbr->min_rtt_timestamp = now_ts;
            bbr->min_rtt_since_last_probe_rtt = -1;
            bbr->app_limited_since_last_probe_rtt = false;
        }
        return min_rtt_expired;
    }

    int SelfToRemoteQosUnit::bbr_should_extend_min_rtt_expiry(bbr_controller_t *bbr) {
        if (bbr->config.probe_rtt_disabled_if_app_limited && bbr->app_limited_since_last_probe_rtt)
            return true;

        if (bbr->config.probe_rtt_skipped_if_similar_rtt && bbr->app_limited_since_last_probe_rtt
            && bbr->min_rtt_since_last_probe_rtt <= bbr->min_rtt * kSimilarMinRttThreshold)
            return true;

        return false;
    }

    void SelfToRemoteQosUnit::bbr_update_recovery_state(bbr_controller_t *bbr, int64_t last_acked_packet, int losses,
                                                        int is_round_start) {
        /*如果发生丢包,recovery就结束，转换到CONSERVATION状态，记录下切换时最后一个被ACK的包序号，保证重新刺探的周期至少一个FEEDBACK时间周期*/
        if (losses)
            bbr->end_recovery_at = last_acked_packet;

        switch (bbr->recovery_state) {
            case NOT_IN_RECOVERY:
                /*发生丢包，切换到CONSERVATION,进行快恢复判断*/
                if (losses) {
                    bbr->recovery_state = CONSERVATION;
                    if (bbr->mode == STARTUP)
                        bbr->recovery_state = bbr->config.initial_conservation_in_startup;

                    bbr->recovery_window = 0;
                    /*标示结束一个round trip周期，启动下一个round trip周期*/
                    bbr->current_round_trip_end = last_acked_packet;
                }
                break;

            case CONSERVATION:
            case MEDIUM_GROWTH:
                if (is_round_start)
                    bbr->recovery_state = GROWTH;

            case GROWTH:
                /*假如没有丢包，而且在保持GROWTH至少一个feedback周期，停止recovery状态*/
                if (!losses && (bbr->end_recovery_at == -1 || bbr->end_recovery_at < last_acked_packet))
                    bbr->recovery_state = NOT_IN_RECOVERY;
                break;
        }
    }

    void SelfToRemoteQosUnit::bbr_update_ack_aggregation_bytes(bbr_controller_t *bbr, int64_t ack_time,
                                                               size_t newly_acked_bytes) {
        size_t expected_bytes_acked;
        int32_t bandwidth;
        if (bbr->aggregation_epoch_start_time == -1)
            return;

        bandwidth = wnd_filter_best(&bbr->max_bandwidth);
        expected_bytes_acked = (size_t) (bandwidth * (ack_time - bbr->aggregation_epoch_start_time));

        if (bandwidth <= 0)
            return;

        if (bbr->aggregation_epoch_bytes <= expected_bytes_acked) {
            bbr->aggregation_epoch_bytes = newly_acked_bytes;
            bbr->aggregation_epoch_start_time = ack_time;
            return;
        }

        bbr->aggregation_epoch_bytes += newly_acked_bytes;
        wnd_filter_update(&bbr->max_ack_height, bbr->aggregation_epoch_bytes - expected_bytes_acked,
                          bbr->round_trip_count);
    }

    /*如果是在probe_bw模式下，尝试进行pacing_gain参数放大，以此来探测最大的带宽*/
    void SelfToRemoteQosUnit::bbr_update_gain_cycle_phase(bbr_controller_t *bbr, int64_t now_ts, size_t prior_in_flight,
                                                          bool losses) {
        int gain_cycling;
        if (gaincyclelength == cycle_len_) {
            gaincyclelength = kGainCycleLength - rand() % bbr_cycle_rand;
            cycle_len_ = 0;
        }

        /*if (bbr->rtt_stat.mean_deviation > kGamma*bbr->rtt_stat.smoothed_rtt) {
            gaincyclelength = gaincyclelength - 3;
        }
        else {
            gaincyclelength = 8;
        }*/
        gain_cycling = (now_ts - bbr->last_cycle_start > bbr_get_min_rtt(bbr)) ? true : false;
        // If the pacing gain is above 1.0, the connection is trying to probe the
        // bandwidth by increasing the number of bytes in flight to at least
        // pacing_gain * BDP.  Make sure that it actually reaches the target, as long
        // as there are no losses suggesting that the buffers are not able to hold
        // that much.
        //std::cout << "prior_in_flight" << prior_in_flight << "bbr_get_target_congestion_window" << bbr_get_target_congestion_window(bbr, bbr->pacing_gain) << std::endl;
        //	std::cout << "losses:" << losses << std::endl;
        if (bbr->pacing_gain > 1.0 &&
            /*!losses &&*/ /*prior_in_flight*/!bbr_check_probe_bw_if_full_bandwidth_reached(bbr,
                                                                                            feedback.feedback.data_in_flight)/*feedback.feedback.data_in_flight < bbr_get_target_congestion_window(bbr, 1.25)*/) //????????????????
            gain_cycling = false;
        // If pacing gain is below 1.0, the connection is trying to drain the extra
        // queue which could have been incurred by probing prior to it.  If the number
        // of bytes in flight falls down to the estimated BDP value earlier, conclude
        // that the queue has been successfully drained and exit this cycle early.	
        //std::cout << "in_flight" << feedback.feedback.data_in_flight << std::endl;
        //std::cout << "BDP" << bbr_get_target_congestion_window(bbr, 1) << std::endl;
        if (bbr->pacing_gain < 1.0 &&
            feedback.feedback.data_in_flight/*prior_in_flight*/ >= bbr_get_target_congestion_window(bbr, 1.25))
            gain_cycling = false;
        if (gain_cycling) {
            bbr->cycle_current_offset = (bbr->cycle_current_offset + 1) % gaincyclelength;
            bbr->last_cycle_start = now_ts;
            cycle_len_++;
            //if (bbr->config.fully_drain_queue && bbr->pacing_gain < 1 /*&& bbr_get_pacing_gain(bbr, bbr->cycle_current_offset) == 1*/
            //	&&feedback.feedback.data_in_flight/*prior_in_flight*/ > 1.25*bbr_get_target_congestion_window(bbr, 1))
            //{
            //	std::cout << "dddddddddddddddddddd" << std::endl;
            //	return;
            //}
            if (bbr->config.fully_drain_queue && bbr->pacing_gain < 1 &&
                bbr_get_pacing_gain(bbr, bbr->cycle_current_offset) == 1
                && prior_in_flight < bbr_get_target_congestion_window(bbr, 1))
                return;

            bbr->pacing_gain = bbr_get_pacing_gain(bbr, bbr->cycle_current_offset);
        }
    }

    /*对STARTUP和DRAIN状态下的带宽进行判断，判断当前带宽是否达到链路最高，如达到最高，设置为带宽充满标示*/
    void SelfToRemoteQosUnit::bbr_check_if_full_bandwidth_reached(bbr_controller_t *bbr) {
        int32_t target;

        /*上层应用暂停了bbr控制模式*/
        if (bbr->last_sample_is_app_limited)
            return;

        target = (int32_t) (bbr->bandwidth_at_last_round * kStartupGrowthTarget);

        if (target <= bbr_bandwidth_estimate(bbr)) { /*并发带宽增长大于预期的增长，说明带宽利用率还有空间，继续进行增长直到到达full bandwidth*/
            bbr->bandwidth_at_last_round = bbr_bandwidth_estimate(bbr);
            bbr->rounds_without_bandwidth_gain = 0;
        } else {
            bbr->rounds_without_bandwidth_gain++;
            if (bbr->rounds_without_bandwidth_gain >= bbr->config.num_startup_rtts
                /*|| (bbr->config.exit_startup_on_loss && bbr_in_recovery(bbr))*/) /*在in recovery状态下可以判定为full bandwidth*/
                bbr->is_at_full_bandwidth = true;
        }
    }

    //新加，对probe_bw状态下的带宽探测进行判断，判断当前带宽是否达到链路最高，如达到最高，设置为带宽充满标示
    bool
    SelfToRemoteQosUnit::bbr_check_probe_bw_if_full_bandwidth_reached(bbr_controller_t *bbr, size_t bytes_inflight) {
        /*上层应用暂停了bbr控制模式*/
        if (bbr->last_sample_is_app_limited)
            return false;
        if (bytes_inflight <=
            bbr_get_target_congestion_window(bbr, 1.25)) { /*并发带宽增长大于预期的增长，说明带宽利用率还有空间，继续进行增长直到到达full bandwidth*/
            bbr->probe_bw_rounds_without_inflight_grow = 0;
        } else {
            bbr->probe_bw_rounds_without_inflight_grow++;
            if (bbr->probe_bw_rounds_without_inflight_grow >= bbr->config.num_probebw_inflight_grows
                /*|| (bbr->config.exit_startup_on_loss && bbr_in_recovery(bbr))*/) /*在in recovery状态下可以判定为full bandwidth*/
            {
                bbr->is_probe_bw_bandwidth_full = true;
                bbr->probe_bw_rounds_without_inflight_grow = 0;
                return true;
            }

        }
        return false;
    }

    void SelfToRemoteQosUnit::bbr_maybe_exit_startup_or_drain(bbr_controller_t *bbr, bbr_feedback_t *feedback) {
        int64_t exit_threshold_ms = bbr->config.exit_startup_rtt_threshold_ms;
        int rtt_over_threshold;

        rtt_over_threshold = (exit_threshold_ms > 0 && bbr->last_rtt - bbr->min_rtt > exit_threshold_ms) ? true : false;
        /*如果是startup模式，它的状态已经是到了带宽最大或者网络延迟超过了阈值，切换至DRAIN模式*/
        //std::cout << "feedback->loss_rate:" << feedback->loss_rate << std::endl;
        if (bbr->mode == STARTUP && (bbr->is_at_full_bandwidth ||
                                     rtt_over_threshold/*||feedback->loss_rate>0.3*/)) {     /////////?????????????????
            bbr->mode = DRAIN;
            bbr->pacing_gain = kDrainGain;
            bbr->congestion_window_gain = kDrainGain;
        }
        /*如果是drain模式，但是正在传输的数据小于当前的拥塞窗口，说明有带宽富余，尝试进入probe_bw模式进行最大带宽探测*/
        if (bbr->mode == DRAIN && feedback->data_in_flight < bbr_get_target_congestion_window(bbr, 1)) {
            bbr_enter_probe_bandwidth_mode(bbr, feedback->Fb_Send_Time);
        }
    }

    void SelfToRemoteQosUnit::bbr_maybe_enter_or_exit_probe_rtt(bbr_controller_t *bbr, bbr_feedback_t *feedback,
                                                                int is_round_start, int min_rtt_expired) {
        /*最近评估的min_rtt过期且当前不在PROBE_RTT模式下，切换到PROBE_RTT模式下*/
        if (bbr->rounds_with_largelossrate >= 2/*bbr->config.num_startup_rtts*/) {
        }
        if ((min_rtt_expired/*||bbr->rounds_with_largelossrate>=2*/) && !bbr->exiting_quiescence &&
            bbr->mode != PROBE_RTT) {
            bbr->mode = PROBE_RTT;
            if (bbr_bandwidth_estimate(bbr) <= 60) {
                bbr->pacing_gain = 0.75;
            } else if (bbr_bandwidth_estimate(bbr) > 60 && bbr_bandwidth_estimate(bbr) <= 150) {
                bbr->pacing_gain = 0.5;
            } else {
                bbr->pacing_gain = 0.5;
            }
            //bbr->round_trip_countprevious = bbr->round_trip_count;
            bbr->exit_probe_rtt_at = -1;
        }

        if (bbr->mode == PROBE_RTT) {
            /*暂停正在发送报文的带宽检测，防止带宽下降太慢而测试min_rtt不准*/
            sampler_on_app_limited(bbr->sampler);
            if (bbr->exit_probe_rtt_at < 0) {
                /*正在发送的数据适配到probe_rtt模式下的拥塞大小，进行最小RTT采集,持续200毫秒*/
                if ((feedback->data_in_flight < bbr_probe_rtt_congestion_window(
                        bbr) /*+ kMaxPacketSize*/)/*|| feedback->data_in_flight <= 1.25*(bbr->sampler->total_data_sent - bbr->sampler->total_data_sent_at_last_acked_packet)*/) {
                    bbr->pacing_gain = 0.9;  //200ms探测最小rtt时保持0.75增益
                    bbr->exit_probe_rtt_at = feedback->Fb_Send_Time + kProbeRttTimeMs;
                    bbr->probe_rtt_round_passed = false;
                    //std::cout << "feedback->data_in_flight" << feedback->data_in_flight << "bbr_probe_rtt_congestion_window(bbr)" << bbr_probe_rtt_congestion_window(bbr) << std::endl;
                }
            } else {
                /*过了一个RTT周期，可以切换probe_rtt*/
                bbr->pacing_gain = 0.9;//////////新增的，当达到inflight<=0.75BDP后，不再进行减速************
                if (is_round_start)
                    bbr->probe_rtt_round_passed = true;
                if (feedback->Fb_Send_Time >= bbr->exit_probe_rtt_at && bbr->probe_rtt_round_passed) {
                    /*probe_rtt完毕，记录评测的生效时间*/
                    bbr->min_rtt_timestamp = feedback->Fb_Send_Time;
                    /*切换BBR的模式*/
                    /////////////////////////////////
                    //bbr->rounds_without_bandwidth_gain = 0;
                    //bbr->is_at_full_bandwidth = false;
                    bbr_check_if_full_bandwidth_reached(bbr);
                    bbr->rounds_with_largelossrate = 0;
                    //////////////////////////////////////////新增
                    if (!bbr->is_at_full_bandwidth)
                        bbr_enter_startup_mode(bbr);
                    else
                        bbr_enter_probe_bandwidth_mode(bbr, feedback->Fb_Send_Time);
                }
            }
        }

        bbr->exiting_quiescence = false;
    }

    void SelfToRemoteQosUnit::bbr_calculate_pacing_rate(bbr_controller_t *bbr, uint32_t bandwidth) {
        int32_t target_rate;
        if (bbr_bandwidth_estimate(bbr) <= 0)
            return;

        target_rate = (int32_t) (bbr->pacing_gain * bbr_bandwidth_estimate(bbr));
        //target_rate = (int32_t)(bbr->pacing_gain * bandwidth);
        if (bbr->config.rate_based_recovery && bbr_in_recovery(bbr))
            bbr->pacing_rate = (int32_t) (bbr->pacing_gain * wnd_filter_third_best(&bbr->max_bandwidth));
        //if (bbr->is_at_full_bandwidth) {
        //	bbr->pacing_rate = bbr_get_congestion_window(bbr) / bbr->rtt_stat.smoothed_rtt;
        //	//std::cout << "bbr->pacing_rate_at_full_bandwidth1" << bbr->pacing_rate << std::endl;
        //	bbr->pacing_rate = SU_MAX(target_rate, bbr->pacing_rate);
        //	//std::cout << "bbr->rtt_stat.smoothed_rtt:" << bbr->rtt_stat.smoothed_rtt <<"bbr_get_congestion_window(bbr)"<< bbr_get_congestion_window(bbr)<< std::endl;
        //    //std::cout << "bbr->pacing_rate_at_full_bandwidth" << bbr->pacing_rate << std::endl;
        //	return;
        //}
        /*开始阶段，用初始化的拥塞窗口计算可以用的码率*/
        if (bbr->pacing_rate == 0 && bbr->rtt_stat.min_rtt > 0) {
            bbr->pacing_rate = (int32_t) (bbr->initial_congestion_window / (bbr->rtt_stat.min_rtt));
            return;
        }

        /*慢启动过程,做一个带宽倍数增*/
        if (bbr->config.slower_startup && bbr->end_recovery_at > 0) {
            bbr->pacing_rate = (int32_t) (kStartupAfterLossGain * bbr_bandwidth_estimate(bbr));
            return;
        }
        bbr->pacing_rate = target_rate;
        //bbr->pacing_rate = SU_MAX(bbr->pacing_rate, target_rate);
    }

    void
    SelfToRemoteQosUnit::bbr_calculate_recovery_window(bbr_controller_t *bbr, size_t bytes_acked, size_t bytes_lost,
                                                       size_t bytes_in_flight) {
        if (bbr->config.rate_based_recovery || bbr->config.rate_based_startup && bbr->mode == STARTUP)
            return;

        /*不在恢复发送阶段，不计算recovery window大小*/
        if (bbr->recovery_state == NOT_IN_RECOVERY)
            return;

        if (bbr->recovery_window == 0) {
            bbr->recovery_window = bytes_in_flight + bytes_acked;
            bbr->recovery_window = SU_MAX(bbr->min_congestion_window, bbr->recovery_window);
            return;
        }

        bbr->recovery_window = ((bbr->recovery_window >= bytes_lost) ? (bbr->recovery_window - bytes_lost)
                                                                     : kMaxSegmentSize);

        /*进行状态加成*/
        if (bbr->recovery_state == GROWTH)
            bbr->recovery_window += bytes_acked;
        else if (bbr->recovery_window == MEDIUM_GROWTH)
            bbr->recovery_window += bytes_acked / 2;

        bbr->recovery_window = SU_MAX(bbr->recovery_window, bytes_in_flight + bytes_acked);
        bbr->recovery_window = SU_MAX(bbr->min_congestion_window, bbr->recovery_window);
    }

    void SelfToRemoteQosUnit::bbr_calculate_congestion_window(bbr_controller_t *bbr, size_t bytes_acked) {
        size_t target_window;
        /*PROBE_RTT模式下不改变拥塞窗口,一般是采用最小窗口模式进行*/
        if (bbr->mode == PROBE_RTT)
            return;

        target_window = bbr_get_target_congestion_window(bbr, bbr->congestion_window_gain);

        if (bbr->rtt_variance_weight > 0 && bbr_bandwidth_estimate(bbr) > 0) {
            target_window += (size_t) (bbr->rtt_variance_weight * bbr->rtt_stat.mean_deviation *
                                       bbr_bandwidth_estimate(bbr));
        } else if (bbr->max_aggregation_bytes_multiplier > 0 && bbr->is_at_full_bandwidth) {
            if (bbr->max_aggregation_bytes_multiplier * wnd_filter_best(&bbr->max_ack_height) >
                bbr->bytes_acked_since_queue_drained / 2)
                target_window += (size_t) (
                        bbr->max_aggregation_bytes_multiplier * wnd_filter_best(&bbr->max_ack_height) -
                        bbr->bytes_acked_since_queue_drained / 2);
        } else if (bbr->is_at_full_bandwidth)
            target_window += (size_t) (wnd_filter_best(&bbr->max_ack_height));

        if (bbr->is_at_full_bandwidth)
            bbr->congestion_window = SU_MIN(target_window, bbr->congestion_window + bytes_acked);
        else if (bbr->congestion_window < target_window ||
                 sampler_total_data_acked(bbr->sampler) < bbr->initial_congestion_window)
            bbr->congestion_window = bbr->congestion_window + bytes_acked;

        bbr->congestion_window = SU_MAX(bbr->congestion_window, bbr->min_congestion_window);
        bbr->congestion_window = SU_MIN(bbr->congestion_window, bbr->max_congestion_window);
    }


    //丢包的处理
    double SelfToRemoteQosUnit::bbr_loss_filter_get(bbr_loss_rate_filter_t *filter) {
        return filter->loss_rate_estimate;
    }

    int
    SelfToRemoteQosUnit::bbr_feedback_get_loss(bbr_feedback_t *feedback, bbr_packet_info_t *loss_arr, int arr_size) {
        int i, count;

        count = 0;
        for (i = 0; i < feedback->packets_num; ++i) {
            if (feedback->packets[i].recv_time <= 0)
                loss_arr[count++] = feedback->packets[i];
        }

        return count;
    }

    void SelfToRemoteQosUnit::bbr_discard_lost_packets(bbr_controller_t *bbr, bbr_packet_info_t packets[], int size) {
        int i;
        for (i = 0; i < size; ++i)
            sampler_on_packet_lost(bbr->sampler, packets[i].seq);
    }

    void SelfToRemoteQosUnit::sampler_on_packet_lost(bbr_bandwidth_sampler_t *sampler, int64_t packet_number) {
        //sampler_remove_point(sampler, packet_number);
    }

    void
    SelfToRemoteQosUnit::bbr_loss_filter_update(bbr_loss_rate_filter_t *filter, int64_t feedback_ts, int packets_sent,
                                                int packets_lost) {
        filter->lost_count += packets_lost;
        filter->total_count += packets_sent;

        /*对丢包比例进行计算*/
        if (filter->next_loss_update_ms + kUpdateIntervalMs < feedback_ts && filter->total_count > kLimitNumPackets) {
            filter->loss_rate_estimate = 1.0f * filter->lost_count / filter->total_count;
            filter->lost_count = 0;
            filter->total_count = 0;

            filter->next_loss_update_ms = feedback_ts;
        }
    }

    //round trip判断
    int SelfToRemoteQosUnit::bbr_update_round_trip_counter(bbr_controller_t *bbr, int64_t last_acked_packet) {
        if (last_acked_packet > bbr->current_round_trip_end ||
            last_acked_packet + PacketSeqloopSize > bbr->current_round_trip_end) {
            bbr->round_trip_count++;
            bbr->current_round_trip_end = bbr->last_sent_packet;
            return true;
        } else
            return false;
    }

    //反馈包处理器
    void SelfToRemoteQosUnit::bbr_feedback_on_feedback(bbr_fb_adapter_t *adapter, bbr_feedback_msg_t *msg) {
        //uint16_t seq;
        //int i;
        ////int64_t now_ts;
        packet_feedback p;

        ////now_ts =   //毫秒级
        //int64_t now_ts = GET_SYS_TIME();

        //adapter->feedback.packets_num = 0;
        //adapter->feedback.feedback_time = now_ts;
        adapter->feedback.prior_in_flight = adapter->hist->outstanding_bytes;
        //seq = msg->samplers[0].seq;
        DWORD headtemp = adapter->hist->l->GetHeadIndex();
        if (adapter->hist->last_ack_seq_sendtime > msg->feedbackpacketsendtime ||
            !adapter->hist->l->IsIndexInValid(msg->last_num)) {
            return; ////////////****************乱序处理，正确性有待确认***********
        }
        while (headtemp != msg->last_num) {
            headtemp = (headtemp + 1) % adapter->hist->l->GetQueueCapacity();
            adapter->hist->l->SetHeadIndex(headtemp);
            adapter->hist->outstanding_bytes -= adapter->hist->l->GetItemByIndex(headtemp)->payload_size;
            //////////////有时突然为0，但是此时lastseq并不等于sendseq
        }
        adapter->hist->last_ack_seq_num = msg->last_num;
        adapter->hist->last_ack_seq_sendtime = msg->feedbackpacketsendtime;
        adapter->feedback.data_in_flight = SU_MAX(adapter->hist->outstanding_bytes, 0);
    }
    int SelfToRemoteQosUnit::bbr_feedback_get_received(bbr_feedback_t *feedback, bbr_packet_info_t *recvd_arr,
                                                       int arr_size) {
        int i, count;

        count = 0;

        for (i = 0; i < feedback->packets_num; ++i) {
            if (feedback->packets[i].recv_time > 0)
                recvd_arr[count++] = feedback->packets[i];
        }

        return count;
    }

    bbr_network_ctrl_update_t
    SelfToRemoteQosUnit::bbrOnFeedback(bbr_controller_t *BBR, bbr_feedback_t *FeedBack, uint32_t bandwidth) {

        int64_t feedback_recv_time, last_acked_packet, fb_send_time;
        int loss_num = 0, acked_num = 0, i;
        float lossrate = 0;
        bbr_packet_info_t *last_sent_packet;
        size_t total_data_acked_before, data_acked_size, data_lost_size;
        int is_round_start = false, min_rtt_expired = false;

        feedback_recv_time = FeedBack->feedback_time;
        fb_send_time = FeedBack->Fb_Send_Time;

        if (FeedBack->packets_num < 0)
            return bbr_create_rate_update(BBR, FeedBack->feedback_time);

        total_data_acked_before = sampler_total_data_acked(BBR->sampler);

        /*对丢包的处理*/
        loss_num = FeedBack->loss_num;
        lossrate = FeedBack->loss_rate;

        acked_num = FeedBack->last_num;
        /*对丢包速率进行评估*/
        bbr_loss_filter_update(&BBR->loss_rate, feedback_recv_time, FeedBack->packets_num, loss_num);
        if (acked_num > 0) {
            last_acked_packet = FeedBack->last_num;
            //point->total_data_acked进行比较得到roundtrip是否增加
            int pos;
            shared_ptr<bbr_packet_point_t> point;
            pos = acked_num;
            point = BBR->sampler->points->QosSend_GetItemByIndex(acked_num);
            if (point->ignore == 1) {
                //LOGGER_WARN(termtotermqos_log, "cannot find in sendhistory:, {}", point->last_acked_packet_ack_time);
            }
            is_round_start = bbr_update_round_trip_counter(BBR, /*point->total_data_acked_at_the_last_acked_packet*/
                                                           acked_num/*last_acked_packet*/);
            if (is_round_start) {
                if (lossrate <= 10) { /*并发带宽增长大于预期的增长，说明带宽利用率还有空间，继续进行增长直到到达full bandwidth*/
                    BBR->rounds_with_largelossrate = 0;
                } else {
                    BBR->rounds_with_largelossrate++;
                }
            }
            min_rtt_expired = bbr_update_bandwidth_and_min_rtt(BBR, feedback_recv_time, fb_send_time, point, bandwidth);

            //bbr_update_recovery_state(BBR, last_acked_packet, loss_num > 0 ? true : false, is_round_start);

            data_acked_size = sampler_total_data_acked(BBR->sampler) - total_data_acked_before;

            bbr_update_ack_aggregation_bytes(BBR, feedback_recv_time, data_acked_size);
            if (BBR->max_aggregation_bytes_multiplier > 0) {
                if (FeedBack->data_in_flight <= 1.25 * bbr_get_target_congestion_window(BBR, BBR->pacing_rate))
                    BBR->bytes_acked_since_queue_drained = 0;
                else
                    BBR->bytes_acked_since_queue_drained += data_acked_size;
            }
        }

        /*PROBE_BW mode*/
        if (BBR->mode == PROBE_BW)
            //进行周期性带宽探测
            bbr_update_gain_cycle_phase(BBR, fb_send_time, FeedBack->prior_in_flight, loss_num > 0 ? true : false);

        /*STARTUP and DRAIN mode*/
        if (is_round_start && !BBR->is_at_full_bandwidth)
            //判断带宽是否充满
            bbr_check_if_full_bandwidth_reached(BBR);
        bbr_maybe_exit_startup_or_drain(BBR, FeedBack);

        /*PROBE_RTT mode,判断是否要进行RTT评估或结束RTT评估*/
        bbr_maybe_enter_or_exit_probe_rtt(BBR, FeedBack, is_round_start, min_rtt_expired);

        /*计算拥塞窗口的大小*/
        data_acked_size = sampler_total_data_acked(BBR->sampler) - total_data_acked_before;
        data_lost_size = 0;
        data_lost_size = loss_num * 1400;   //这里假定丢失包大小都为1400

        bbr_calculate_pacing_rate(BBR, bandwidth);// ?????????/
        bbr_calculate_congestion_window(BBR, data_acked_size);
        bbr_calculate_recovery_window(BBR, data_acked_size, data_lost_size, FeedBack->data_in_flight);

        /*带宽统计向前移动*/
        if (acked_num > 0)
            sampler_remove_old(BBR->sampler, acked_num, FeedBack->packetseqroundtrip);
        return bbr_create_rate_update(BBR, FeedBack->feedback_time);
    }

    void SelfToRemoteQosUnit::bbr_on_network_invalidation() {
        size_t outstanding;
        double fill;
        uint8_t loss;
        uint32_t pacing_rate_kbps, target_rate_bps, pading_rate_kbps, instant_rate_kbps;
        int acked_bitrate;

        if (info.congestion_window <= 0)
            return;
        /*设置pace参数*/

        /*计算反馈带宽*/
        outstanding = feedback.hist->outstanding_bytes;
        instant_rate_kbps = info.congestion_window / info.target_rate.rtt;
        target_rate_bps = (SU_MIN(info.target_rate.target_rate, instant_rate_kbps) * 8000);
        acked_bitrate = bbr_feedback_get_bitrate(&feedback);
        fill = 1.0 * outstanding / info.congestion_window;
        /*如果拥塞窗口满了，进行带宽递减*/
        if (fill > 1.0) {
            encoding_rate_ratio = 0.9f;
            if (acked_bitrate > 0)
                target_bitrate = acked_bitrate * encoding_rate_ratio;
            else
                target_bitrate = target_bitrate * encoding_rate_ratio;
        } else {
            encoding_rate_ratio = 1;
            if (fill < 0.9)
                target_bitrate = target_bitrate + SU_MIN(32 * 1000, SU_MAX(min_bitrate / 32, 4 * 8000));
        }
        //target_bitrate = SU_MIN(target_rate_bps, target_bitrate);
        target_bitrate = SU_MAX(target_bitrate, min_bitrate);
        target_bitrate = SU_MIN(target_bitrate, max_bitrate);

        loss = m_fraction_loss;

        TermRoundTripTimeInfo term_roundtriptime;
        term_roundtriptime.termtotermrtt.roundtriptime = bbr->min_rtt;
        term_roundtriptime.termtotermrtt.bandwidth= bbr_bandwidth_estimate(bbr);
        term_roundtriptime.m_remote_tid = m_d_remote_tid;
        //TEST
        TermSpeedInfo term_speed_info;
        term_speed_info.m_remote_tid = m_d_remote_tid;
        term_speed_info.m_d_speed = info.target_rate.target_rate * 8000;
#ifndef QOS_ShowBwRTT
        if (IsStart) {
            LastTime = GetTickCount();
            IsStart = false;
        }

        if ((GetTickCount() - LastTime) > 1000) {
            if (IsSatellite && bbr->min_rtt > 4000) {
                //SHOW_WARN(single_hop_delay_log, "The single hop delay is: {}ms,  hop count is: {}", (bbr->min_rtt % 2000 + 1900) / (2 * hop_count), hop_count);
            }
            if (!IsSatellite && bbr->min_rtt > 1000) {
                //SHOW_WARN(termtotermqos_congestion_control_log, "one way delay: {}ms", (bbr->min_rtt % 2000 + 1900) / 2);
                //SHOW_WARN(single_hop_delay_log, "The single hop delay is: {}ms,  hop count is: {}", (bbr->min_rtt % 2000 + 1900) / (2 * hop_count), hop_count);
            }
            //GGER_WARN(termtotermqos_congestion_control_log, "one way delay: {}ms", bbr->min_rtt / 2);
            //SHOW_WARN(single_hop_delay_log, "The single hop delay is: {}ms,  hop count is: {}", bbr->min_rtt / (2 * hop_count), hop_count);
            /*if (bbr->min_rtt <= 100) {
                SHOW_WARN(termtotermqos_congestion_control_log, "bbr->min_rtt:, {}", bbr->min_rtt / (2* hop_count));
            }
            else {
                _SHOW_WARN(termtotermqos_congestion_control_log, "bbr->min_rtt:, {}", (bbr->min_rtt / 2) % 50 + 10);
            }*/
            LastTime = GetTickCount();
        }
#endif
        SpeedControlQoSSpeedInfo(term_speed_info);    //target_bitrate单位为bps
        //printf("[QOSDebug]::change Speed to %f kps\n",term_speed_info.m_d_speed/1000);
        QoSRoundTripTimeInfo(make_shared<TermRoundTripTimeInfo>(term_roundtriptime));
        if (target_bitrate != last_bitrate_bps || last_fraction_loss) {
            last_bitrate_bps = target_bitrate;
            last_rtt = info.target_rate.rtt;
            last_fraction_loss = loss;
        }

    }

}

