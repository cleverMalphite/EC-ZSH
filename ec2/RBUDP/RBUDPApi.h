//o
//// Created by Kong n 2023/6/9.
//
#include "SerializeUtil_RBUDP.h"

#ifndef RBUDP_API
#define RBUDP_API

//------------------------回调函数接口定义------------------------
//用于BigDataTransfer的回调
typedef void(*PGETDATAFROMRBUDPFORBIGDATATRANSFERCALLBACK)(
        unsigned long int dTermId,
        const std::shared_ptr<unsigned char> pRecvDataBytes,
        unsigned long int dRecvDataByteLength
);

//对外通用的数据服务回调接口定义-
//20240911-houlc RBUDP_DATASERVER_CALLBACK
//typedef void(*RBUDP_DATASERVER_CALLBACK)(
//       DWORD dTermId,
//       const std::shared_ptr<BYTE> &recv_data,
//       DWORD dRecvDataByteLength
//);

//---------------------------注册回调函数-------------------------
//注册数据回调函数(BigDataTransfer用)
bool RegisterRBUDPFunc(PGETDATAFROMRBUDPFORBIGDATATRANSFERCALLBACK pFunc);

////20240911-houlc register_rbudp_dataserver_callback
////对外通用的注册数据服务回调的注册函数
////注意：需要注册服务类型的名称+回调函数
////  以便接收数据时根据名称判断数据类型后,
////  分发到的响应的回调函数中
//bool register_rbudp_dataserver_callback(std::string server_name,
//                            RBUDP_DATASERVER_CALLBACK p_callback);


//-----------------------模块核心功能函数-------------------------
//初始哈RBUDP模块
void InitRBUDP(std::string sSendFileDirPath);
//释放资源模块
RBUDP_API void UnInitRBUDP();
//可靠传输发送函数
bool SendDataBytesToTermByRBUDP(DWORD dTermId, 
                                const std::shared_ptr<BYTE> &bytes,
                                DWORD nLength);
//不可靠传输发送函数
//@Return 如果终端dTermId上线，那么返回true，否则返回false
RBUDP_API bool SendDataBytesToTermByRBUDPWithoutReliable(
                                DWORD dTermId, 
                                std::shared_ptr<BYTE> bytes,
                                DWORD nLength);
//----------------------模块内工具函数------------------------
//调用上层模块注册到本模块的各个数据回调函数
void CallBackDataFunc(
              DWORD dTermId, 
              std::shared_ptr<BYTE> pRecvDataBytes,
              DWORD dRecvDataByteLength);

////数据服务回调的数据检查函数
////20240911-houlc for spin_server_callback
////typedef void(*RBUDP_DATASERVER_CALLBACK)(
//bool rbudp_dataserver_check(
//              const std::shared_ptr<BYTE> &recv_data,
//              DWORD dRecvDataByteLength );

////数据回调服务对应的回调调用
////20240911-houlc spin_server_callback
////typedef void(*RBUDP_DATASERVER_CALLBACK)(
////        DWORD dTermId,
////        const std::shared_ptr<BYTE> &recv_data,
////        DWORD dRecvDataByteLength
////);
//bool rbudp_spin_server_callback(DWORD dTermId,
//                const std::shared_ptr<BYTE> &recv_data,
//                DWORD dRecvDataByteLength) ;


#endif
