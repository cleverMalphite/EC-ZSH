#pragma once

#include "bbr_controller.h"
#include "InterfaceDataAbstractBuffer.h"
#include "../epollComm/CIPSOCKET.h"
#include "DataCodec.h"

namespace Term2TermQos
{
    /**
     * 远端发送给本端的反馈包
     */
    class Remote2SelfPacketQosFeedbackPacket
            : public InterfaceDataAbstractBuffer
    {
    public:
        Remote2SelfPacketQosFeedbackPacket(DWORD d_term_tid);
        Remote2SelfPacketQosFeedbackPacket() { m_d_remote_tid = 0; }
        virtual ~Remote2SelfPacketQosFeedbackPacket() {}
        // 通过 InterfaceDataAbstractBuffer 继承
        virtual shared_ptr<BYTE> Encode(DWORD & d_data_length) override;
        virtual bool Decode(shared_ptr<BYTE> p_buffer, DWORD d_data_length) override;
        void    Add_Feedback_Msg(bbr_feedback_msg_t* msg);     //添加反馈包编码信息
        void     SetRecvTime(int64_t &recvtime) { Recv_Time = recvtime; }
        void     SetFb_SendTime(int64_t &fb_sendtime) { fb_send_time = fb_sendtime; }
        DWORD     GetRemoteTermId() const { return m_d_remote_tid; }
        DWORD     GetSelfTermId()   const { return m_d_self_tid; }
        uint8_t   GetFlag()         const { return loss_flag; }
        float   GetLoss()         const { return fraction_loss; }
        uint8_t   GetLoss_rate()     const { return loss_rate; }
        int       GetLossNum()      const { return loss_num; }
        int       GetPacketNum()    const { return packet_num; }
        uint32_t  GetRecvRate()     const { return recv_bitrate; }
        int       GetLastNum()      const { return last_num; }
        int Getpacketseqroundtrip()   const { return packetseqroundtrip; }
        uint32_t   GetData_acked()    const { return data_acked; }
        int64_t   GetLastNumSendTime()    const { return last_num_recv_time; }
        int64_t   GetFb_packet_sendts()   const { return fb_packet_send_time; }
        int       GetDeltaTs()      const { return delta_ts; }
        int64_t   GetRecvTime()     const { return Recv_Time; }
        int64_t   GetFb_Send_Time()     const { return fb_send_time; }
        static bool IsInstanceOf(shared_ptr<BYTE> pBuffer, DWORD d_data_length);	//如果pBuffer对应的是此反馈包，那么返回true，否则返回false
    private:
        const static BYTE m_d_comman_flag = 3;
        int64_t                 Recv_Time;        //接收到反馈数据包的时间戳
        int64_t                 fb_send_time;        //反馈数据包发送的时间戳
        DWORD                   m_d_remote_tid;	  //远端终端号
        DWORD                   m_d_self_tid;     //本端终端号
        uint8_t					loss_flag;        //loss info msg
        float					fraction_loss;    //丢包率
        uint8_t                 loss_rate;
        int						loss_num;       //收到的包数
        int                     packet_num;     //数据包总数
        uint32_t                recv_bitrate;
        int				        last_num;      //反馈包中数据信息个数
        int                     packetseqroundtrip;
        uint32_t                data_acked;
        int64_t                 last_num_recv_time;//数据包从发送端到接收端netcomm模块的时间戳，后面再经过封包形成反馈包返回
        int64_t                 fb_packet_send_time;//生成反馈数据包时间戳
        int                     delta_ts;


    };
}