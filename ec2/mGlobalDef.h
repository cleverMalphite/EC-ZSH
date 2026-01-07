//
// Created by 王炳棋 on 2022/11/14.
//
#include <cstring>
#include <memory>
#include <climits>
#include <queue>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>
#include <algorithm>
#include "Util/SystemTimeFunc.h"

#ifndef VIDEOREANS_TEST
#define VIDEOREANS_TEST

inline FILE *fp_DataWithPacketInfo_send;
inline FILE *fp_DataWithPacketInfo_recv;
inline FILE *fp_DataCodec_send;
inline FILE *fp_DataCodec_recv;
inline FILE *fp_channel_send;
inline FILE *fp_channel_recv;

inline std::string filename_DataWithPacketInfo_send="DataWithPacketInfo_send_video.h264";
inline std::string filename_DataWithPacketInfo_recv="DataWithPacketInfo_recv_video.h264";
inline std::string filename_DataCodec_send="DataCodec_send_video.h264";
inline std::string filename_DataCodec_recv="DataCodec_recv_video.h264";
inline std::string filename_channel_send="channel_send_video.h264";
inline std::string filename_channel_recv="channel_recv_video.h264";

#endif

//通道状态查询等待间隔
#ifndef ENUM_NETTERMINAL_CHANNEL_STATUS
#define ENUM_NETTERMINAL_CHANNEL_STATUS
#define STATE_TRAVERSE_DELAY_TIME 10000

//#define NetCombTransfer_Test_Log
//#define COMBTRANSFER_CHANNELTRAVERSE
//#define COMBTRANSFER_ROUTERINFO
enum enumNetTerminalChannelStatus {
    E_NETCOMB_STATUS_CHANNEL_INVALID = 0,    //无效通道，初始值
    E_NETCOMB_STATUS_CHANNEL_CONNECTED = 1,    //通道连接完成
    E_NETCOMB_STATUS_CHANNEL_RID_CONFIRMED = 2,    //通道身份确认
    E_NETCOMB_STATUS_RUDPPORT_CONFIRMED = 3,    //UDP通道端口协商完成
    E_NETCOMB_STATUS_CHANNEL_NEGOTIATED = 4,    //通道协商完成
};
#endif //ENUM_NETTERMINAL_CHANNEL_STATUS

#ifndef NETCOMBTRANSFER_MGLOBALDEF_H
#define NETCOMBTRANSFER_MGLOBALDEF_H

typedef unsigned int UINT;
typedef unsigned char BYTE;
typedef unsigned short WORD;
//DWORD是32位的
typedef unsigned int DWORD;
//long在linux中是8字节的大小
typedef unsigned long DWORD64;

#ifndef RELEASE_ARRAY
#define RELEASE_ARRAY

//为了扭曲的实现两个智能指针同时指向一个区域，而且其中一个智能指针所指内存区域不断变化，另一个智能指针指向区域不变的
//情况，我们需要那个指向的内存经常变化的智能指针的析构什么也不做
template<typename T>
void releaseArraysDoNothing(T const *p) {
}

//定义一个数组的内存释放函数模板，用来辅助shared_ptr
template<typename T>
void releaseArrays(T *p) {
    if (NULL == p || nullptr == p) {
        return;
    }
    delete[]    p;
    p = NULL;
    //TODO;
}

#endif //RELEASE_ARRAY

#ifndef MRUDP_FIRST_BYTE_FLAG
#define MRUDP_FIRST_BYTE_FLAG

#define MRUDP_DATA_FIRST_BYTE_FLAG 8
#define MRUDP_MESSAGE_FIRST_BYTE_FLAG 9
#define MRUDP_DATA_FIRST_BYTE_FLAG_WITHOUT_RELIABLE 10
#define MRUDP_FORCE_SPEED_MESSAGE 11
#define MRUDP_FORCE_ADJUSTBANDWIDTH_MESSAGE 12
#define MRUDP_KEEPALIVE_MESSAGE 13
#define MRUDP_VIDEO_TRANS 15
//视频传输数据类型
#define MRUDP_VIDEO_TRANS_DATA 16

#endif //MRUDP_FIRST_BYTE_FLAG

#ifndef VIDEO_TRANS_START_END_FLAG
#define VIDEO_TRANS_START_END_FLAG

//视频传输信令的具体类型，开始请求视频传输or通知结束视频传输
#define VIDEO_TRANS_START 20
#define VIDEO_TRANS_END 21
#define VIDEO_TRANS_BUSY 22

#endif //VIDEO_TRANS_START_END_FLAG

#ifndef RESERVE_CHANNEL_BEGIN_END
#define RESERVE_CHANNEL_BEGIN_END

#define MAXCHANNELNUM 10000
//保留的客户端通道号，专供ThirdPartyTransfer之类的外部模块使用
#define RESERVE_CHANNEL_BEGIN 6500
#define RESERVE_CHANNEL_END   6599

#endif //RESERVE_CHANNEL_BEGIN_END


#ifndef RBUDP_FIRST_BYTE_FLAG
#define RBUDP_FIRST_BYTE_FLAG
#define RBUDP_DATA_FIRST_BYTE_FLAG 8
#define RBUDP_MESSAGE_FIRST_BYTE_FLAG 9
#define RBUDP_DATA_FIRST_BYTE_FLAG_WITHOUT_RELIABLE 10
#define RBUDP_FORCE_SPEED_MESSAGE 11
#define RBUDP_FORCE_ADJUSTBANDWIDTH_MESSAGE 12
#define RBUDP_KEEPALIVE_MESSAGE 13
#endif

class MRUDPTransferReason {
public:
    const static BYTE TRANSFER_FOR_INIT = 1;    //首次重传
    const static BYTE TRANSFER_FOR_REQUEST = 2; //请求重传
    const static BYTE TRANSFER_FOR_TIMEOUT = 3; //超时重传
};

class MRUDPAckKind {
public:
    const static BYTE UNSACKED = 1;    //还未接到任何确认
    const static BYTE SACKED = 2; //得到了SACK确认
    const static BYTE CUMULATIVE_ACKED = 3;    //得到了累积确认
};

namespace Util {
    const static std::string INVALID_IP_ADDRESS = "0";    //无效IP地址
}

/**
	 * 用来标志循环队列队首队尾元素状态
	 */
enum CircularDataQueueIndexStatus {
    /* 分以下几种情况：
    * 1. 队列首索引比队列尾索引小；
    * 2. 队列首索引比队列尾索引大；
    * 3. 队列首索引与队列尾索引相等，且队列有效长度为0；
    * 4. 队列首索引与队列尾索引相等，且队列有效长度为队列容量大小；
    * 5. 索引错误状态（不属于上述状态之一）
    */
    Queue_Head_Index_Less_Than_Queue_Tail_Index,
    Queue_Head_Index_Greater_Than_Queue_Tail_Index,
    Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Full,
    Queue_Head_Index_Equal_To_Queue_Tail_Index_Size_Zero,
    Wrong_Head_Tail_Index_Status
};

#endif //NETCOMBTRANSFER_MGLOBALDEF_H

