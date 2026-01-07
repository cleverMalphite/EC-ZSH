
#include "Remote2SelfQosUnit.h"
#include "../NetCombTransfer/NetCombTransferApi.h"
#include "SpeedControlApi.h"

namespace Term2TermQos
{
    typedef struct SendToRemoteData
    {
        SendToRemoteData(DWORD d_remote_tid, shared_ptr<BYTE> s_data, DWORD d_length) :
                m_d_remote_tid(d_remote_tid), m_data(s_data), m_data_length(d_length)
        {}
        DWORD m_d_remote_tid = 0;
        shared_ptr<BYTE> m_data = nullptr;
        DWORD m_data_length = 0;
    }SendToRemoteData;


    //TODO:Leak_definitelyLost
    Remote2SelfQosUnit::Remote2SelfQosUnit(DWORD d_self_tid, DWORD d_remote_tid) :m_d_self_tid(d_self_tid), m_d_remote_tid(d_remote_tid)
    {
        pthread_mutex_init(&m_cs, nullptr);
        ///在此处进行接收端初始化
        base_seq = -1;
        first_packet = 0;
        first_recv_packet_index = 0;
        packetcount = 0;
        storgepacketsnumber = new ReceivdCircularDataQueue();
        packetseqroundtrip = 0;
        //bin_stream_init(&strm);                  ///还没有销毁
        loss_statistics_init(&loss_stat);
        cache = skiplist_create(id64_compare, nullptr, nullptr);   ////还没有销毁
        //printf("[OOMDebug]::Remote2SelfQosUnit Create, cache:%d\n",cache);
    }
    Remote2SelfQosUnit::~Remote2SelfQosUnit()
    {
        pthread_mutex_destroy(&m_cs);
        //释放先前通过skiplist_create(3)中calloc分配的内存
        delete storgepacketsnumber;
        //printf("[OOMDebug]::Remote2SelfQosUnit Destroy,cache : %d\n",cache);
        free(cache);
    }
    void Remote2SelfQosUnit::RecvRemoteToSelfQosInfo(shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_qos_info)
    {
        MutexLockGuard gGuard(m_cs);
        
        if (nullptr == p_remote_to_self_qos_info)
        {
            return;
        }

        m_remote_to_self_qos_info_queue.push(p_remote_to_self_qos_info);
    }

/*
#ifdef DEAL_CAPACITY_DealRemoteToSelfQosInfo
    string speedcontrol_send_unit_name("DealRemoteToSelfQosInfo");
	DWORD32 speedcontrol_send_unit_cycle = 10000;
	void SpeedControlSendRate(string name, DWORD32 statics)
	{
		std::cout << name << ":" << statics << std::endl;;
	}
	StatisticsCapacity g_speedcontrol_statistics(speedcontrol_send_unit_name, 
                                                 speedcontrol_send_unit_cycle, 
                                                 SpeedControlSendRate);
#endif // DEAL_CAPACITY_DealRemoteToSelfQosInfo
*/
       
    void Remote2SelfQosUnit::DoCycleTime()
    {
        DealRemoteToSelfQosInfo();
/*
#ifdef  DEAL_CAPACITY_DealRemoteToSelfQosInfo
        g_speedcontrol_statistics.ComputeOnce();
#endif // DEAL_CAPACITY_SendTIDDataWithSpeedControl
*/

    }
    //	void Remote2SelfQosUnit::DealRemoteToSelfQosInfo()
    //	{
    //		queue<shared_ptr<SendToRemoteData> > p_send_data_queue;
    //		//临界区**************************************************************************
    //		pthread_mutex_lock(&m_cs);
    //		using Term2TermQos::Remote2SelfPacketQosFeedbackPacket;
    //
    //		bbr_feedback_msg_t msg;
    //		int64_t sequence, now_ts;
    //		uint16_t seq;
    //		skiplist_iter_t* iter;
    //		skiplist_item_t key, val;
    //		now_ts = GET_SYS_TIME();
    //		//TODO ADD 从 m_remote_to_self_qos_info_queue中取出数据进行处理，生成反馈包并进行相应回调
    //		size_t s = m_remote_to_self_qos_info_queue.size();
    //		if (s == 0)
    //		{
    //			pthread_mutex_unlock(&m_cs);
    //			return;
    //		}
    //
    //		shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_info = m_remote_to_self_qos_info_queue.front();
    //		// 从队列中删除一个数据包
    //		m_remote_to_self_qos_info_queue.pop();
    //
    //		seq = p_remote_to_self_info->m_d_index;
    //		send_time = p_remote_to_self_info->m_d_timestamp;
    //		count = p_remote_to_self_info->m_d_datalength;
    //		first_packet++;
    //		if (first_packet == 1)
    //		{
    //			last_recv_time = now_ts;
    //		}
    //		loss_statistics_incoming(&loss_stat, seq);
    //
    //		if (seq > base_seq + 32767)
    //		{
    //			pthread_mutex_unlock(&m_cs);
    //			return;
    //		}
    //		base_seq = SU_MAX(base_seq, seq);
    //		key.i64 = seq;
    //		accumulated_count += count;
    //		if (skiplist_search(cache, key) == NULL)
    //		{
    //			val.i64 = now_ts;
    //			//skiplist_insert(cache, key, val);
    //		}
    //
    //		active_wnd_size = now_ts - last_recv_time;
    //
    //		if (active_wnd_size > 1000)
    //		{
    //			first_packet = 0;
    //			acked_bitrate = (int)(accumulated_count / active_wnd_size + 0.5);
    //			shared_ptr< Remote2SelfPacketQosFeedbackPacket> pFeedback = make_shared<Remote2SelfPacketQosFeedbackPacket>();
    //			msg.feedback_index = 3;       //反馈包标志位
    //			msg.nDeviceID = p_remote_to_self_info->m_d_remote_tid;
    //			msg.nRemoteID = m_d_self_tid;
    //			msg.flag = 2;                //BBR：消息bbr_acked_msg
    //			msg.last_num = seq;
    //			msg.last_num_send_time = p_remote_to_self_info->m_d_timestamp;
    //			msg.delta_ts = active_wnd_size;
    //			msg.acked_bitrate = acked_bitrate;
    //			std::cout << "acked_bitrate:" << acked_bitrate << "kBps" << std::endl;
    //
    //			accumulated_count = 0;
    //
    //			if (loss_statistics_calculate(&loss_stat, now_ts, &msg.fraction_loss, &msg.loss_num, &msg.packet_num) == 0)
    //				msg.flag = 1;     //bbr_loss_info_msg
    //			//编码丢包反馈包并发送
    //			pFeedback->Add_Feedback_Msg(&msg);
    //			std::shared_ptr<BYTE> feedback_buffer = nullptr;
    //			DWORD FeedbackLength = 0;
    //			feedback_buffer = pFeedback->Encode(FeedbackLength);
    //			//调用NetCombTransfer的发送数据接口
    //			shared_ptr<SendToRemoteData> p_send_data = make_shared<SendToRemoteData>(m_d_remote_tid, feedback_buffer, FeedbackLength);
    //			p_send_data_queue.push(p_send_data);
    //			msg.sampler_num = 0;
    //		}
    //		skiplist_clear(cache);
    //
    //		pthread_mutex_unlock(&m_cs);
    //		shared_ptr<SendToRemoteData> p_send_data = nullptr;
    //		while (!p_send_data_queue.empty())
    //		{
    //			shared_ptr<SendToRemoteData> p_send_data = p_send_data_queue.front();
    //			if (nullptr != p_send_data)
    //			{
    //#ifndef QOS_TEST
    //				SendTIDDataWithSpeedControl(p_send_data->m_d_remote_tid, p_send_data->m_data, p_send_data->m_data_length);
    //#endif
    //#ifdef QOS_TEST
    //				bool bOK = SendTIDDataWithSpeedControl(p_send_data->m_d_remote_tid, p_send_data->m_data, p_send_data->m_data_length);
    //				if (bOK)
    //				{
    //					std::cout << "反馈包发送成功!" << std::endl;
    //				}
    //				else
    //				{
    //					std::cout << "反馈包发送失败!" << std::endl;
    //				}
    //#endif
    //			}
    //			p_send_data_queue.pop();
    //		}
    //	}
    //
    void Remote2SelfQosUnit::DealRemoteToSelfQosInfo()
    {
        //printf("Run Remote2SelfQosUnit::DealRemoteToSelfQosInfo()\n");
        queue<shared_ptr<SendToRemoteData> > p_send_data_queue;
        //临界区**************************************************************************
        pthread_mutex_lock(&m_cs);
        using Term2TermQos::Remote2SelfPacketQosFeedbackPacket;
        bbr_feedback_msg_t msg;
        int64_t sequence, now_ts;
        uint16_t seq;
        shared_ptr<RemoteToSelfQosInfo> p_remote_to_self_infotemp=make_shared<RemoteToSelfQosInfo>();
        //int64_t seq;
        skiplist_iter_t* iter;
        skiplist_item_t key, val;
        now_ts = GET_SYS_TIME();
        //printf("\n[QOSDebug]::DealRemoteToSelfQosInfo Now_ts is %ld\n\n",now_ts);
        //TODO ADD 从 m_remote_to_self_qos_info_queue中取出数据进行处理，生成反馈包并进行相应回调
        size_t s = m_remote_to_self_qos_info_queue.size();
        if (s == 0)
        {
            //if (!is_recv_start) {
            pthread_mutex_unlock(&m_cs);
            //cout << "queue empty" << endl;
            return;
            //}
            /*else {
                m_remote_to_self_qos_info_queue.push(p_remote_to_self_info_last_time);
            }*/

        }
        shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_info = m_remote_to_self_qos_info_queue.front();
        m_remote_to_self_qos_info_queue.pop();
        p_remote_to_self_infotemp->m_d_datalength = p_remote_to_self_info->m_d_datalength;
        p_remote_to_self_infotemp->m_d_index = p_remote_to_self_info->m_d_index;
        p_remote_to_self_infotemp->m_d_remote_tid = p_remote_to_self_info->m_d_remote_tid;
        p_remote_to_self_infotemp->m_d_timestamp = p_remote_to_self_info->m_d_timestamp;
        p_remote_to_self_infotemp->recv_timestamp = p_remote_to_self_info->recv_timestamp;
        seq = p_remote_to_self_info->m_d_index;
        send_time = p_remote_to_self_info->m_d_timestamp;
        count = p_remote_to_self_info->m_d_datalength;
        accumulated_count += count;
        //cout << "count" << count << endl;
        //if (count != 0) {
        packetcount++;
        //}
        first_packet++;
        if (first_packet == 1)
        {
            last_recv_time = now_ts;
            first_recv_packet_index = seq;
            //is_recv_start = true;
        }
        if (!storgepacketsnumber->Qos_DealSingleReceiveData(p_remote_to_self_infotemp)) {
            packetcount--; }
        //loss_statistics_incoming(&loss_stat, seq);
        if (seq > base_seq + 32767)
        {
            pthread_mutex_unlock(&m_cs);
            return;
        }
        base_seq = SU_MAX(base_seq, seq);
        key.i64 = seq;
        //if (skiplist_search(cache, key) == NULL)
        //{
        //	val.i64 = now_ts;
        //skiplist_insert(cache, key, val); //改动：将接收包存入跳表中
        //}
        //SPDLOG_LOGGER_WARN(termtotermqos_log, "packet received is:, {}", seq);
        //将上个接收包信息保存起来
        /*p_remote_to_self_info_last_time->m_d_datalength = 0;
        p_remote_to_self_info_last_time->m_d_index = p_remote_to_self_info->m_d_index;
        p_remote_to_self_info_last_time->m_d_remote_tid = p_remote_to_self_info->m_d_remote_tid;
        p_remote_to_self_info_last_time->m_d_timestamp = p_remote_to_self_info->m_d_timestamp;
        p_remote_to_self_info_last_time->recv_timestamp=p_remote_to_self_info->recv_timestamp;
        last_recv_time_test = now_ts;*/
        active_wnd_size = now_ts - last_recv_time;  //改动二
        if (/*first_packet>=50*/active_wnd_size >= 10/*storgepacketsnumber.size() >= 3*//*MAX_BBR_FEEDBACK_COUNT*/) //改动：更改为每隔128个数据包进行一次反馈
        {
            auto lastseq = storgepacketsnumber->Qos_GetTailIndex();///////
            msg.loss_num = storgepacketsnumber->m_queue_size - packetcount;
            if (msg.loss_num < 0) {
                msg.loss_num = 0;
            }
            msg.packet_num = first_packet;////待修改的
            first_packet = 0;
            packetcount = 0;
            acked_bitrate = (int)(accumulated_count / (active_wnd_size+0.01) + 0.5);//改动：重新更改间隔时间，接收最后一个包时间减去第一个数据包接收时间   测试时有时这里会提示除数为0
            shared_ptr< Remote2SelfPacketQosFeedbackPacket> pFeedback = make_shared<Remote2SelfPacketQosFeedbackPacket>();
            msg.feedback_index = 3;       //反馈包标志位
            msg.nDeviceID = p_remote_to_self_info->m_d_remote_tid;
            msg.nRemoteID = m_d_self_tid;
            msg.accumulated_count = accumulated_count;
            //msg.nRemoteID = p_remote_to_self_info->m_d_remote_tid;
            msg.flag = 2;                //BBR：消息bbr_acked_msg
            msg.last_num = seq;
            msg.packetseqroundtrip = packetseqroundtrip;
            msg.last_num_recv_time = p_remote_to_self_info->recv_timestamp;
            msg.feedbackpacketsendtime = now_ts;
            //printf("\n[QOSDebug]::DealRemoteToSelfQosInfo feedbackpacketsendtime is %ld\n\n",msg.feedbackpacketsendtime);
            msg.delta_ts = active_wnd_size/*recent_time - last_recv_time*/;////改动：重新更改间隔时间，接收最后一个包时间减去第一个数据包接收时间
            msg.acked_bitrate = acked_bitrate;
            //msg.loss_num = 0;////改动：加入msg.sampler_num计数
            accumulated_count = 0;
            ///////改动： 将丢失的包信息写入msg.samplers[]数组中  ////
            ///SKIPLIST_FOREACH(cache, iter)
            //if (m_remote_to_self_qos_info_queue.size() > 5) { SPDLOG_LOGGER_WARN(termtotermqos_log, "recv data size:, {}", m_remote_to_self_qos_info_queue.size()); }
            float lossrate = msg.loss_num * 10000 / (/*msg.loss_num +*/ storgepacketsnumber->m_queue_size + 1);
            msg.fraction_loss = lossrate/100;
#ifdef QOS_ShowLoss
            if ((lossrate / 100) > 60) {
				SPDLOG_LOGGER_WARN(termtotermqos_log, "first packet recv_time:, {}", last_recv_time);
				SPDLOG_LOGGER_WARN(termtotermqos_log, "first_recv_packet_index:, {}", first_recv_packet_index);
				SPDLOG_LOGGER_WARN(termtotermqos_log, "recv packet:, {}", msg.packet_num);

			}
			SPDLOG_LOGGER_WARN(termtotermqos_log, "loss count:, {}", msg.loss_num);
			SPDLOG_LOGGER_WARN(termtotermqos_log, "loss  rate:, {}", lossrate / 100);
			if ((lossrate / 100) < 60) {
				SPDLOG_LOGGER_WARN(termtotermqos_congestion_control_log, "The receiver loss  rate is:, {}", lossrate / 100);
			}
#endif

            //if (loss_statistics_calculate(&loss_stat, now_ts, &msg.fraction_loss, &msg.loss_num, &msg.packet_num) == 0)
            //	msg.flag = 1;     //bbr_loss_info_msg
            //编码丢包反馈包并发送
            /*SPDLOG_LOGGER_WARN(termtotermqos_congestion_control_log, "last feedback packet sendtime:, {}", previous_feedback_sendtimestamp);
            SPDLOG_LOGGER_WARN(termtotermqos_congestion_control_log, "this time feedback packet sendtime:, {}", p_remote_to_self_info->m_d_timestamp);
            SPDLOG_LOGGER_WARN(termtotermqos_congestion_control_log, "adjacent feedback sendtime interval:, {}", p_remote_to_self_info->m_d_timestamp - previous_feedback_sendtimestamp);
            previous_feedback_sendtimestamp = p_remote_to_self_info->m_d_timestamp;
*/
            pFeedback->Add_Feedback_Msg(&msg);
            std::shared_ptr<BYTE> feedback_buffer = nullptr;
            DWORD FeedbackLength = 0;
            feedback_buffer = pFeedback->Encode(FeedbackLength);
            //调用NetCombTransfer的发送数据接口
            shared_ptr<SendToRemoteData> p_send_data = make_shared<SendToRemoteData>(m_d_remote_tid, feedback_buffer, FeedbackLength);
            p_send_data_queue.push(p_send_data);
            msg.loss_num = 0;
            storgepacketsnumber->Qos_SetHeadIndex(storgepacketsnumber->Qos_GetTailIndex());////////////////改动：清除容器内容
            if (storgepacketsnumber->m_queue_size != 0) {
                //std::cout << "the size of storgepacketsnumber is:" << storgepacketsnumber->m_queue_size << std::endl;
            }
            skiplist_clear(cache);
        }

        pthread_mutex_unlock(&m_cs);
        shared_ptr<SendToRemoteData> p_send_data = nullptr;
        while (!p_send_data_queue.empty())
        {
            shared_ptr<SendToRemoteData> p_send_data = p_send_data_queue.front();
            if (nullptr != p_send_data)
            {
                //Test_GetFeedbackPacketFromNetCombTransfer(100, p_send_data->m_data, p_send_data->m_data_length, false);
#ifndef QOS_TEST
                int64_t oldts = GET_SYS_TIME();
                SendTIDDataWithoutSpeedControl(p_send_data->m_d_remote_tid, p_send_data->m_data, p_send_data->m_data_length,
                                               false);
                //SendTIDCommandWithoutSpeedControl(p_send_data->m_d_remote_tid, p_send_data->m_data, p_send_data->m_data_length);
#endif
#ifdef QOS_TEST
                bool bOK = SendTIDDataWithoutSpeedControl(p_send_data->m_d_remote_tid, p_send_data->m_data, p_send_data->m_data_length);
				if (bOK)
				{
					std::cout << "反馈包发送成功!" << std::endl;
				}
				else
				{
					std::cout << "反馈包发送失败!" << std::endl;
				}
#endif
            }
            p_send_data_queue.pop();
        }
    }




    //下面是QoS的函数
    int32_t Remote2SelfQosUnit::bin_stream_init(void* ptr)
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
    }
    void Remote2SelfQosUnit::loss_statistics_init(cc_loss_statistics_t* loss_stat)
    {
        loss_stat->count = 0;
        loss_stat->max_id = -1;
        loss_stat->prev_max_id = -1;
        loss_stat->stat_ts = -1;
    }
    void Remote2SelfQosUnit::loss_statistics_incoming(cc_loss_statistics_t* loss_stat, uint16_t seq)
    {
        int64_t id;
        id = seq;
        if (loss_stat->prev_max_id == -1 && id > 0)
            loss_stat->prev_max_id = id - 1;
        if (loss_stat->max_id < id)
            loss_stat->max_id = id;
        ++loss_stat->count;
    }
    int Remote2SelfQosUnit::loss_statistics_calculate(cc_loss_statistics_t* loss_stat, int64_t now_ts, uint8_t* fraction_loss, int* loss_num, int* num)
    {
        int disance;
        float loss_rate;

        *fraction_loss = 0;
        *loss_num = 0;
        *num = 0;

        //loss_evict_oldest(loss_stat, now_ts);

        if (loss_stat->max_id == -1 || loss_stat->prev_max_id == -1
            || (now_ts < loss_stat->stat_ts + k_loss_statistics_window_ms || loss_stat->max_id < loss_stat->prev_max_id + 128))
            return -1;

        disance = loss_stat->max_id - loss_stat->prev_max_id + 1;
        if (disance <= 0)
            return -1;

        if (disance <= loss_stat->count)
        {
            *fraction_loss = 0;

        }
        else {
            *fraction_loss = (disance - loss_stat->count) * 255 / disance;
            loss_rate = (disance - loss_stat->count) * 100 / disance;
            int a = (int)*fraction_loss;
        }
        *loss_num = disance - loss_stat->count;
        *num = disance;
#ifdef QOS_TEST
        std::cout << "the disance:" << disance << "   " << "the loss rate is:" << loss_rate << std::endl;
#endif // QOS_TEST
        loss_stat->prev_max_id = loss_stat->max_id;
        loss_stat->count = 0;
        loss_stat->stat_ts = now_ts;

        return 0;
    }

}
