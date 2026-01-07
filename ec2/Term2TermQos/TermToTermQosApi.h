#pragma once
/*#include "outGeneral.h"*/
#include "../mGlobalDef.h"
//#include "QosStructInterfaceInfo.h"
#include "../SpeedControl/QosStructInterfaceInfo.h"

#ifndef TERMTOTERMQOS_API
using namespace std;
typedef void(*pMRUDPGetQosRTTInfoCallBack)(const DWORD d_remote_tid);
bool InitTermToTermQos(const DWORD d_self_tid);
bool UninitTermToTermQos();
TermToTermRoundTripTime MRUDP_Get_QosRtt(const DWORD d_remote_tid);
//TODO ADD 本模块注册到速率控制模块以为其提供端到端带宽估计的接口
bool RateInfo(DWORD m_remote_tid, DWORD m_d_speed);

void QoSRoundTripTimeInfo(shared_ptr<TermRoundTripTimeInfo> term_rtt_info);
//TODO TEST 供测试用的本模块导出函数，实际上这些函数应该作为本模块的回调函数
//注册到NetCombTransfer模块的终端上下线回调函数
bool RegisterRttSimpleCallBack(pRttCallBackSimple qos_rtt_callback);

void Test_TermOnOrOff(DWORD dwCID, bool bCreate);
//注册到NetComTransfer模块获得远端反馈包的函数
bool Test_GetFeedbackPacketFromNetCombTransfer(DWORD dwTID, shared_ptr<BYTE> pBuffer, DWORD dwLength, bool bMsg);
//接收端
void Test_RecvRemoteToSelfQosInfo(shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_qos_info);
//发送端
void Test_RecvSelfToRemoteQosInfo(shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info);
//TermSpeedInfo Test_getspeedinfo();

#endif