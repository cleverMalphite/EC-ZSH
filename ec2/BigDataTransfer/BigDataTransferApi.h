//
// Created by 王炳棋 on 2023/1/29.
//

#ifndef NETCOMBTRANSFER_BIGDATATRANSFERAPI_H
#define NETCOMBTRANSFER_BIGDATATRANSFERAPI_H
/*#pragma once
#ifndef BIGDATATRANSFER_API_H
#define BIGDATATRANSFER_API_H*/

#include <memory>
#include <string>
#include "TempSDKInterfaceParams.h"
#include "BigDataTransferManager.h"

using std::shared_ptr;
#ifndef P_RegisterBigDataTransferCallBack
#define P_RegisterBigDataTransferCallBack



typedef void(*pRegisterFileSendTaskProgressCallBack)(
        const std::shared_ptr<FileTaskSendProgressInfo> &
);

typedef void(*pRegisterFileRecvTaskProgressCallBack)(
        const std::shared_ptr<FileTaskRecvProgressInfo> &
);

typedef void(*pRegisterFileSendTaskStatusCallBack)(
        const std::shared_ptr<FileTaskSendStatusInfo> &
);

typedef void(*pRegisterFileRecvTaskStatusCallBack)(
        const std::shared_ptr<FileTaskRecvStatusInfo> &
);

#endif//P_RegisterBigDataTransferCallBack

/**
 * 导出函数，初始化此模块
 */
bool InitBigDataTransfer();

/**
 * 导出函数，卸载本模块
 */
bool UninitBigDataTransfer();

/**
 * 导出函数，方便其它模块测试本模块
 */
bool Create_BigDataTransferTask(DWORD RemoteTID,
                                const std::string &s_file_name,
                                const std::string &s_file_absolute_name
                                , DWORD &TaskID);

bool Create_BigDataTransferTask(DWORD RemoteTID,
                                const std::string &s_file_name,
                                const std::string &s_file_absolute_name,
                                DWORD &TaskID,
                                bool is_RBUDP);
//供SDK模块调用的获得所有文件传输任务发送带宽的API
std::shared_ptr<FileSendBandWidth> BigDataTransfer_GetAllFileSendBandWidth();

//供SDK模块调用的获得所有文件传输任务接收带宽的API
std::shared_ptr<FileRecvBandWidth> BigDataTransfer_GetAllFileRecvBandWidth();

//取消某个文件发送任务
bool SetFileSendTaskCancel(DWORD task_id);

//**************供VISUALIZATION模块调用的主动式回调函数注册API**********************************
bool RegisterFileSendTaskProgressCallBack(pRegisterFileSendTaskProgressCallBack pFunc);

bool RegisterFileRecvTaskProgressCallBack(pRegisterFileRecvTaskProgressCallBack pFunc);
//************************************************************************************

//**************供VISUALIZATION模块调用的被动式回调函数注册API**********************************
bool RegisterFileSendTaskStatusCallBack(pRegisterFileSendTaskStatusCallBack pFunc);

bool RegisterFileRecvTaskStatusCallBack(pRegisterFileRecvTaskStatusCallBack pFunc);
//************************************************************************************


/*#endif//BIGDATATRANSFER_API_H*/
#endif //NETCOMBTRANSFER_BIGDATATRANSFERAPI_H
