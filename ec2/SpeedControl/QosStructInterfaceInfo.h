//
// Created by 王炳棋 on 2022/12/6.
//
#include "../mGlobalDef.h"
#include <cstdint>
#include <queue>

#ifndef SPEEDCONTROL_QOSSTRUCTINTERFACEINFO_H
#define SPEEDCONTROL_QOSSTRUCTINTERFACEINFO_H
#ifndef QOSSTRUCTINFO
#define QOSSTRUCTINFO
/**
 * 这个结构体是本端发给远端的数据包Qos信息（包序号、时间戳）的抽象
 * 本端发给远端的数据包在流经NetCombTransfer模块并经由NetCombTransfer模块发送到远端前，会被NetCombTransfer截取包序号和时间戳等信息，
 * 且NetCombTransfer模块会将这些信息封装为SelfToRemoteQosInfo的一个实例返回给本端的QoS模块。
 */
typedef struct SelfToRemoteQosInfoStruct
{
    DWORD m_d_remote_tid = 0;	//远端终端号
    DWORD m_d_index = 0;		//包序号
    int64_t m_d_timestamp = 0;		//时间戳
    DWORD m_d_data_length = 0;	//数据包大小
} SelfToRemoteQosInfo;
//queue<TermSpeedInfo> speedinfo;//????????????????????????????????
/**
 * 本端的NetCombTransfer模块在接收到远端的数据包后，
 * 会截取数据包的包序号、时间戳等信息，并将这些信息封装为一个RemoteToSelfQosInfo实例，将此实例反馈给本模块。
 */
typedef struct RemoteToSelfQosInfoStruct
{
    DWORD m_d_remote_tid = 0;	//远端终端号
    DWORD m_d_index = 0;		//包序号
    int64_t m_d_timestamp = 0;		//时间戳
    DWORD m_d_datalength = 0;     //新增一个包长度
    int64_t recv_timestamp = 0;
}RemoteToSelfQosInfo;

typedef void(*pRttCallBackSimple)(int rtt_data);
/**
 * 这个头文件主要用于声明和定义一些模块间交互信息
 */

/**
* 远端终端上下线信息
*/

typedef struct TermOnOffInfo
{
    DWORD m_remote_tid = 0;	//远端终端终端号
    bool m_b_on = false;				//为true表示终端上线，为false表示终端下线，默认为false
} TermOnOffInfo;


typedef struct TermToTermRoundTripTime {
    int64_t roundtriptime = 5;  //本端到对应终端的链路的物理往返时延大小
    int64_t bandwidth=0;        //QOS估计的链路的带宽
}TermToTermRoundTripTime;


typedef struct TermRoundTripTimeInfo
{
    DWORD m_remote_tid = 0;	//远端终端终端号
    TermToTermRoundTripTime termtotermrtt;
} TermRoundTripTimeInfo;
/**
 * 端到端数据发送速率控制消息
 */

typedef struct TermSpeedInfo
{
    DWORD m_remote_tid = 0;	//远端终端终端号
    float m_d_speed = 0;	//本端向对应远端发送数据时的期待数据发送速率
    //int64_t m_d_speed = 0;
} TermSpeedInfo;
#endif


#endif //SPEEDCONTROL_QOSSTRUCTINTERFACEINFO_H
