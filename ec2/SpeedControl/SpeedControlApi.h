#ifndef SPEEDCONTROL_LIBRARY_H
#define SPEEDCONTROL_LIBRARY_H

#include "QosStructInterfaceInfo.h"
#include <memory>



//---------------------------------回调函数接口定义-------------------------
//获得从远端接收到的数据(供上层模块用)
typedef bool(*pSpeedControlRecvDataCallBack)(
                          DWORD dwTID, 
                          const std::shared_ptr<BYTE> &pBuffer, 
                          DWORD dwLength,
                          long int &recvtime, 
                          long int &fb_send_time);
//获取双端通信包信息的回调函数，下面两个注册函数应该顾名思义了(供QOS模块用)
typedef void(*pSpeedControlRemoteToSelfQosInfoCallBack)(
                          std::shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_qos_info);

typedef void(*pSpeedControlSelfToRemoteQosInfoCallBack)(
                          std::shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info);
//终端上下线函数，目前仅供QOS模块使用
typedef void(*pSpeedControlTermOnOffCallBack)(
                          DWORD dwTID, bool bCreate);
//供RBUDP获取哪些数据已经从SpeedControl中发送出去
typedef void(*pSpeedControlRBUDPDataCallBack)(
                          DWORD dwTID, DWORD Index);

//------------------------------注册回调函数-------------------------------------------
bool RegisterSpeedControlRBUDPDataCallBack(pSpeedControlRBUDPDataCallBack pFunc);

bool Register_SpeedControl_RecvDataCallBack(pSpeedControlRecvDataCallBack pFunc);

bool RegisterSpeedControlQoSSelf2RemoteInfoCallBack(pSpeedControlSelfToRemoteQosInfoCallBack pFunc);

bool RegisterSpeedControlQoSRemote2SelfInfoCallBack(pSpeedControlRemoteToSelfQosInfoCallBack pFunc);

bool RegisterSpeedControlTermOnOffCallBack(pSpeedControlTermOnOffCallBack pFunc);

//-----------------------------核心API函数---------------------------------------------

bool InitSpeedControl(DWORD n_tid);
bool UninitSpeedControl();

//向指定终端发送数据(附带速率控制)
bool SendTIDDataWithSpeedControl(
            DWORD dwTID, 
            const std::shared_ptr<BYTE> &pBuffer, 
            DWORD dwLength, 
            bool isUrgent = false,
            bool isSleep = false/*为true则强制休眠1毫秒*/, 
            bool isRBUDP = false);

//向指定终端发送数据(无速率控制)
bool SendTIDDataWithoutSpeedControl(
            DWORD dwTID, 
            const std::shared_ptr<BYTE> &pBuffer, 
            DWORD dwLength, 
            bool isRBUDP);
//向指定终端发送数据(无速率控制)
bool SendTIDCommandWithoutSpeedControl(
            DWORD dwTID, 
            const std::shared_ptr<BYTE> &pBuffer, 
            DWORD dwLength, 
            bool isRBUDP);

//-------------------------------QOS对接函数-------------------------------
//供测试用的，相当于QoS的模拟
//传入本端->远端的QOS信息(供QOS用)
void QoSSelf2RemoteInfo(std::shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info);
//传入本端<-远端的QOS信息(供QOS用)
void QoSRemote2SelfInfo(std::shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_qos_info);
//传入本端->远端的期望速率(供QOS用)
bool SpeedControlQoSSpeedInfo(TermSpeedInfo term_speed_info);

//-------------------------------速率设置函数(供用户调用)-------------------------------
bool ExChangeTransSpeedOfTID(DWORD nTID, DWORD nExpectSpeed);

//-------------------------------链路状态查询函数(供上层自适应控制调用)-------------------------------
bool GetSpeedControlQueueStats(DWORD nTID, DWORD &pendingPackets, DWORD &capacityPackets);

#endif //SPEEDCONTROL_LIBRARY_H
