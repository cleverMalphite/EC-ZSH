//
// Created by 王炳棋 on 2022/12/7.
//
#include <utility>

#include "../GlobalMessage.h"
#include "QosStructInterfaceInfo.h"

#ifndef NETCOMBTRANSFER_DATAWITHPACKETINFO_H
#define NETCOMBTRANSFER_DATAWITHPACKETINFO_H
using std::shared_ptr;
using std::make_shared;
/**
 * 这个类主要负责：
 * 1. 封装带QoS反馈信息的数据。
 * 2. 为上层待发送端到端数据加上对端QoS需要的信息
 * 3. 对对端发送的端到端数据进行解码，将数据部分反馈给上层模块（包括QoS模块），同时为QoS模块提供包头信息。
 * 
 * 结构：
 * 循环包序号（4个字节） + 时间戳（8个字节） + 数据部分长度（4个字节） + 数据部分
 */

namespace SpeedControl
{
    class DataWithPacketInfo
    {
    public:
        DataWithPacketInfo(DWORD dest_tid):m_dest_tid(dest_tid) {}
        DataWithPacketInfo(DWORD dest_tid, const SelfToRemoteQosInfo& self_to_remote,
                           std::shared_ptr<BYTE> data, const DWORD data_length,bool isRBUDP=false);
    private:
        const DWORD m_dest_tid = 0;
        const static BYTE m_d_feedback_comman_flag = 3;
        shared_ptr<SelfToRemoteQosInfo> m_data_packet_info_self_to_remote = make_shared<SelfToRemoteQosInfo>();	//意义请参见SelfToRemoteQosInfo类的定义，此成员仅被序列化
        shared_ptr<RemoteToSelfQosInfo> m_data_packet_info_remote_to_self = make_shared<RemoteToSelfQosInfo>();	//意义请参见RemoteToSelfQosInfo类的定义，次成员仅被反序列化
        std::shared_ptr<BYTE> m_data = nullptr;
        DWORD m_data_length = 0;
        bool m_isRBUDP=false;
        static const DWORD MIN_PACKET_DATA_LENGTH = 20;
    public:
        /**
         * 解码函数，将s_data里的内容解码后赋给调用此方法的实例的成员变量
         * 其中dLength是s_data中有效数据部分的长度
         */
        bool Decode(shared_ptr<BYTE> s_data, DWORD dLength);
        bool IsFeedBackInstanceOf(shared_ptr<BYTE> pBuffer, DWORD dLength);
        /**
         * 编码函数，将此方法调用方实例的成员变量序列化为字节序列，
         * 序列化所得的字节序列的有效数据长度通过参数dLength返回
         */
        std::shared_ptr<BYTE> Encode(DWORD& dLength);

        const DWORD GetRemoteTID() { return m_dest_tid; }

        shared_ptr<BYTE> GetData() { return m_data; }
        DWORD GetDataLength() { return m_data_length; }
        shared_ptr<RemoteToSelfQosInfo> GetRemoteToSelfQoSInfo() { return m_data_packet_info_remote_to_self; }
        shared_ptr<SelfToRemoteQosInfo> GetSelfToRemoteQosInfo() { return m_data_packet_info_self_to_remote; }
        bool GetWhichFunc(){return m_isRBUDP;}
    };
}

#endif //NETCOMBTRANSFER_DATAWITHPACKETINFO_H
