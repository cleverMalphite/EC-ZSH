
#include "Remote2SelfPacketQosFeedbackPacket.h"

namespace Term2TermQos
{
    typedef struct UtcTimeStampInQoS
    {
        DWORD dwHighDateTime = 0;
        DWORD dwLowDateTime = 0;
    } UtcTimeStampInQoS;

    //获取64位UTC时间(毫秒)
    UtcTimeStampInQoS su_get_sys_time_ms_qos(int64_t &utc_stamp_get) {
        //timeval timeNow;
        /*gettimeofday(&timeNow, nullptr);
        utc_stamp_get = (timeNow.tv_sec * 1000000 + timeNow.tv_usec)/1000;*/
        struct UtcTimeStampInQoS timeStamp;
        int64_t int_temp_first = utc_stamp_get;
        int64_t int_temp_second = utc_stamp_get;
        timeStamp.dwLowDateTime = int_temp_first & 0x00000000ffffffff;
        timeStamp.dwHighDateTime = (int_temp_second >> 32) & 0x00000000ffffffff;
        return timeStamp;
    }

    Remote2SelfPacketQosFeedbackPacket::Remote2SelfPacketQosFeedbackPacket(DWORD d_term_tid) :m_d_remote_tid(d_term_tid)
    {

    }

    shared_ptr<BYTE> Remote2SelfPacketQosFeedbackPacket::Encode(DWORD & d_data_length)
    {
        int i;
        //给返回的数据分配内存空间
        d_data_length = 1 + 2 * 4 + 1 + 12 * 4;
        std::shared_ptr<BYTE> encodeChar(new BYTE[d_data_length], releaseArrays<BYTE>);

        //开始顺序写入各个字段的值到上述字节序列中
        BYTE* pTemp = encodeChar.get();

        //表示这是反馈包
        pTemp[0] = 3;	                    pTemp += 1;         //反馈包标志位
        WriteData(pTemp, m_d_remote_tid);	pTemp += 4;			//写入当前对端终端号
        WriteData(pTemp, m_d_self_tid);	    pTemp += 4;			//写入当前本端终端号
        WriteData(pTemp, loss_flag);		pTemp += 1;	        //写入丢失标志
        WriteData(pTemp, last_num);	        pTemp += 4;			//写入区间最后一个包序号
        WriteData(pTemp, packetseqroundtrip);	        pTemp += 4;
        //WriteData(pTemp, fraction_loss);	pTemp += 4;			//写入丢包率
        //WriteData(pTemp, packet_num);       pTemp += 4;         //写入包总数
        ////std::cout << "last_num" << last_num << std::endl;
        UtcTimeStampInQoS utc_time_stamp = su_get_sys_time_ms_qos(last_num_recv_time);	//获得64位UTC时间戳
        //printf("[FeedBackPacketEncode]::dwHighDateTime is %d , dwLowDateTime is %d\n",utc_time_stamp.dwHighDateTime,utc_time_stamp.dwLowDateTime);
        WriteData(pTemp, utc_time_stamp.dwHighDateTime);	pTemp += 4;   //时间戳高四位
        WriteData(pTemp, utc_time_stamp.dwLowDateTime);		pTemp += 4;	//时间戳低四位
        UtcTimeStampInQoS utc_time_stampfb = su_get_sys_time_ms_qos(fb_packet_send_time);
        WriteData(pTemp, utc_time_stampfb.dwHighDateTime);	pTemp += 4;   //时间戳高四位
        WriteData(pTemp, utc_time_stampfb.dwLowDateTime);		pTemp += 4;	//时间戳低四位
        WriteData(pTemp, loss_num);	        pTemp += 4;			//写入丢失包数
        WriteData(pTemp, fraction_loss);	        pTemp += 4;			//写入丢包率
        //	//std::cout << "packet_num" << packet_num << std::endl;
        WriteData(pTemp, packet_num);	        pTemp += 4;			//写入丢失包数
        WriteData(pTemp, recv_bitrate);	    pTemp += 4;			//写入接收速率
        WriteData(pTemp, data_acked);	    pTemp += 4;
        WriteData(pTemp, delta_ts);	        pTemp += 4;			//写入时间差
        return encodeChar;
    }
    bool Remote2SelfPacketQosFeedbackPacket::Decode(shared_ptr<BYTE> p_buffer, DWORD d_data_length)
    {
        int i;
        //Recv_Time = GET_SYS_TIME();       //在解码时获得一个接收时间戳
        BYTE*  pTemp = p_buffer.get();
        //读取反馈包标志,如果反馈包buffer长度大于18，进行解码
        if (p_buffer && d_data_length >= 1 + 2 * 4 + 1 + 12 * 4)
        {
            if (pTemp[0] != 3)
            {
                return false;
            }
            pTemp += 1;
            m_d_remote_tid = ReadData(pTemp);	pTemp += 4;	//读取对端终端号
            m_d_self_tid = ReadData(pTemp);	    pTemp += 4;	//读取本端终端号
            loss_flag = ReadData(pTemp);	    pTemp += 1;	//读取丢失标志
            last_num = ReadData(pTemp);         pTemp += 4; //读取区间最后一个包序号
            packetseqroundtrip = ReadData(pTemp);         pTemp += 4;
            //fraction_loss = ReadData(pTemp);	pTemp += 4;	//读取接收数据包数
            //loss_num = ReadData(pTemp);	        pTemp += 4;	//读取反馈信息数
            //packet_num = ReadData(pTemp);       pTemp += 4; //读取包总数
            /////std::cout << "lastnumremote:" << last_num << std::endl;
            UtcTimeStampInQoS utc_stamp_temp;
            utc_stamp_temp.dwHighDateTime = ReadData(pTemp); pTemp += 4;
            //printf("[FeedBackPacketDecode]::dwHighDateTime is %d\n",utc_stamp_temp.dwHighDateTime);
            ////std::cout << "utc_stamp_temp.dwHighDateTime" << utc_stamp_temp.dwHighDateTime << std::endl;
            utc_stamp_temp.dwLowDateTime = ReadData(pTemp); pTemp += 4;
            //printf("[FeedBackPacketDecode]::dwLowDateTime is %d\n",utc_stamp_temp.dwLowDateTime);
            ////std::cout << "utc_stamp_temp.dwLowDateTime" << utc_stamp_temp.dwLowDateTime << std::endl;
            last_num_recv_time = ((((int64_t)utc_stamp_temp.dwHighDateTime)) << 32) | utc_stamp_temp.dwLowDateTime;
            UtcTimeStampInQoS utc_stamp_tempfb;
            utc_stamp_tempfb.dwHighDateTime = ReadData(pTemp); pTemp += 4;
            ////std::cout << "utc_stamp_tempfb.dwHighDateTime" << utc_stamp_tempfb.dwHighDateTime << std::endl;
            utc_stamp_tempfb.dwLowDateTime = ReadData(pTemp); pTemp += 4;
            ////std::cout << "utc_stamp_tempfb.dwLowDateTime" << utc_stamp_tempfb.dwLowDateTime << std::endl;
            fb_packet_send_time = ((((int64_t)utc_stamp_tempfb.dwHighDateTime)) << 32) | utc_stamp_tempfb.dwLowDateTime;
            /////std::cout << "last_num_send_time" << last_num_send_time << std::endl;
            loss_num = ReadData(pTemp);         pTemp += 4; //读取区间最后一个包序号
            fraction_loss = ReadData(pTemp);         pTemp += 4; //读取丢包率信息
            packet_num = ReadData(pTemp);         pTemp += 4;//读取确认数据包总数
            //	//std::cout << "decode packet_num" << packet_num << std::endl;
            recv_bitrate = ReadData(pTemp);	    pTemp += 4;	//读取接收速率
            data_acked = ReadData(pTemp);	    pTemp += 4;
            delta_ts = ReadData(pTemp);         pTemp += 4; //读取时间差
        }
        return true;
    }
    bool Remote2SelfPacketQosFeedbackPacket::IsInstanceOf(shared_ptr<BYTE> pBuffer, DWORD d_data_length)
    {
        if (nullptr != pBuffer && d_data_length > 1)
        {
            BYTE* pTemp = pBuffer.get();
            if (m_d_comman_flag == pTemp[0])
            {
                return true;
            }
        }
        return false;
    }

    void Remote2SelfPacketQosFeedbackPacket::Add_Feedback_Msg(bbr_feedback_msg_t* msg)
    {
        //将结构体msg中的信息转存，用于反馈包的编码
        int i;
        m_d_remote_tid = msg->nRemoteID;
        m_d_self_tid = msg->nDeviceID;
        loss_flag = msg->flag;
        fraction_loss = msg->fraction_loss;
        loss_num = msg->loss_num;
        data_acked = msg->accumulated_count;
        packet_num = msg->packet_num;
        ////std::cout << "msg->packet_num" << packet_num << std::endl;
        recv_bitrate = msg->acked_bitrate;
        last_num = msg->last_num;
        packetseqroundtrip = msg->packetseqroundtrip;
        ////std::cout << "msg->last_num" << msg->last_num << std::endl;
        last_num_recv_time = msg->last_num_recv_time;
        fb_packet_send_time = msg->feedbackpacketsendtime;
        delta_ts = msg->delta_ts;
    }

}
