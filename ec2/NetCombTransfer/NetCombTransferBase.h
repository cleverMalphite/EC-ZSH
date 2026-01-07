//
// Created by 王炳棋 on 2022/11/15.
//

#ifndef NETCOMBTRANSFER_NETCOMBTRANSFERBASE_H
#define NETCOMBTRANSFER_NETCOMBTRANSFERBASE_H

#include "../GlobalMessage.h"

#define  INVALID_DEVICE_ID -1
//定义此模块表示网卡IP地址时使用的是String还是CString
#define IP_ADDRESS_NETCOMBTRANSFER  std::string
//建立一条UDP通道的最大尝试次数
#define CREATE_UDP_CHANNEL_MAX_TRY_TIMES 100
//互连的两终端间TCP服务端和UDP服务端之间的通道号差值（UDP服务端通道号大）
#define TCP_UDP_SERVER_CHANNEL_INTERVAL 1000
//互连的两终端间TCP客户端和UDP客户端之间的通道号差值（UDP客户端通道号大）
#define TCP_UDP_CLIENT_CHANNEL_INTERVAL 2000
//规定此模块最大的尝试初始化次数
#define NETCOMBTRANSFER_MAX_INIT_TIMES 10
//宏定义实现全局临时通道状态流转功能的线程的定时时间间隔
#define NETCOMBTRANSFER_THREAD_DELAY_TIME 3000


//这个枚举里定义了各种命令的消息类型标识
enum enumCommandID {
    eCommandInvalid = 0,

    eCommandChannelStatusNotify = 1000,    //通道状态改变
    eCommandRemoteIDRequest = 1001,    //请求对端TID号码
    eCommandRemoteIDNotify = 1002,    //发送TID号码给对端

    eCommandRemoteUDPPortRequest = 1003,    //请求对端UDP端口号
    eCommandRemoteUDPPortNotify = 1004,    //发送本地的UDP端口号给对端

    eCommandRouterInfoNotify = 1005,    //收到路由增加更新通知
    eCommandRouterInfoDeleteNotify = 1006,    //收到路由删除更新通知

    eCommandShortMessage = 1007, //上层使用

    eCommandSetFileTransferBandWidth = 1008        //设置传输文件的带宽
};


#endif //NETCOMBTRANSFER_NETCOMBTRANSFERBASE_H
