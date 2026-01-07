//
// Created by 王炳棋 on 2022/12/28.
//

#ifndef MRUDP_API
#define MRUDP_API

#include "SerializeUtil.h"
#include <string>
#include <functional>

//------------------回调函数接口定义----------------------------
//用于BigDataTransfer的数据回调
typedef void(*PGETDATAFROMMRUDPFORBIGDATATRANSFERCALLBACK)(
        DWORD dTermId,
        const std::shared_ptr<BYTE> &pRecvDataBytes,
        DWORD dRecvDataByteLength
);

//对外通用的数据服务回调接口定义 
//20240909-houlc MRUDP_DATASERVER_CALLBACK
typedef void(*MRUDP_DATASERVER_CALLBACK)(
        DWORD dTermId,
        const std::shared_ptr<BYTE> &recv_data, 
        DWORD dRecvDataByteLength
);
typedef void(*MRUDP_DATASERVER_CALLBACK2)(
        DWORD dTermId,
        const std::shared_ptr<BYTE> &recv_data, 
        DWORD dRecvDataByteLength,
        void* handle
);
//typedef std::function<void(DWORD, const std::shared_ptr<BYTE>&, DWORD)> MRUDP_DATASERVER_CALLBACK ;
//-----------------------注册回调函数---------------------------
//注册数据回调函数
bool RegisterMRUDPFunc(PGETDATAFROMMRUDPFORBIGDATATRANSFERCALLBACK pFunc);

//20240909-houlc register_mrudp_dataserver_callback
//对外通用的注册数据服务回调的注册函数
//注意：需要注册服务类型的名称+回调函数
//  以便接收数据时根据名称判断数据类型后,
//  分发到的响应的回调函数中
bool register_mrudp_dataserver_callback(std::string server_name,
                            MRUDP_DATASERVER_CALLBACK p_callback);

bool register_mrudp_dataserver_callback(std::string server_name,
                            MRUDP_DATASERVER_CALLBACK2 p_callback, void* handle=nullptr);


//-----------------------模块核心函数---------------------------
//初始化MRUDP模块
void InitMRUDP(const std::string& sSendFileAbosultePath);
//释放资源函数
void UnInitMRUDP();
//供用户调用以发送数据
//@Return 如果数据放入发送缓冲区成功，那么返回true；
//如果返回false，这意味着RUDP的发送缓冲区已满，需要用户做处理
bool SendDataBytesToTermByMRUDP(
              DWORD dTermId, 
              const std::shared_ptr<BYTE>& bytes, 
              DWORD nLength);
//供用户调用以发送数据
//最终效果： 用户向对端发送bytes中从0开始的nLength个元素。
//@Return 如果终端dTermId上线，那么返回true，否则返回false
bool SendDataBytesToTermByMRUDPWithoutReliable(
              DWORD dTermId, 
              std::shared_ptr<BYTE> bytes, 
              DWORD nLength);
//----------------------辅助功能函数----------------------------
//发送信令，让对端进行大规模紧急重传
bool SendAdjustBandWidthMessage( DWORD remote_tid);    
//发送端进行手动重传
bool AdjustselfBandWidth( DWORD remote_tid);   
//改变远端的发送速率？
bool ChangeRemoteSendSpeed( DWORD remote_tid, 
                            DWORD m_force_speed);
//打印MRUDP信息(BigDataTransfer用)
void PrintMRUDPInfo();
//----------------------模块内工具函数----------------------------
//获得某个RUDP的丢包率统计
float ComputeAndGetRUDPLossRateByTID(DWORD d_remote_tid);

//判断终端是否在线：如果在线，那么返回true，否则返回false
bool IsRUDPOnByTermId(DWORD term_id);    

//调用上层模块注册到本模块的各个数据回调函数
void CallBackDataFunc( DWORD dTermId, 
                      const std::shared_ptr<BYTE> &pRecvDataBytes,
                      DWORD dRecvDataByteLength);

//数据服务回调的数据检查函数
//20240911-houlc for spin_server_callback
//typedef void(*MRUDP_DATASERVER_CALLBACK)(
bool dataserver_check(
        const std::shared_ptr<BYTE> &recv_data, 
        DWORD dRecvDataByteLength );

//数据回调服务对应的回调调用
//20240909-houlc spin_server_callback
//typedef void(*MRUDP_DATASERVER_CALLBACK)(
//        DWORD dTermId,
//        const std::shared_ptr<BYTE> &recv_data,
//        DWORD dRecvDataByteLength
//);
bool spin_server_callback(DWORD dTermId,
                      const std::shared_ptr<BYTE> &recv_data,
                      DWORD dRecvDataByteLength) ;


#endif //MRUDP_API
