//
// Created by 王炳棋 on 2023/1/12.
//

#ifndef NETCOMBTRANSFER_FILETRANSFERTASKSTATE_H
#define NETCOMBTRANSFER_FILETRANSFERTASKSTATE_H

#include "../mGlobalDef.h"

#ifndef FILE_TRANSFER_TASK_STATE
#define FILE_TRANSFER_TASK_STATE
//文件发送任务的状态
const static DWORD FILE_SEND_TASK_ING = 0;        //文件正在发送
const static DWORD FILE_SEND_TASK_SUCCESS = 1;    //文件发送成功
const static DWORD FILE_SEND_TASK_FAIL = 2;        //文件发送失败
const static DWORD FILE_SEND_TASK_CANCEL = 3;        //文件接收任务取消

//文件接收任务的状态
const static DWORD FILE_RECV_TASK_ING = 4;        //文件正在接收中
const static DWORD FILE_RECV_TASK_SUCCESS = 5;    //文件接收成功
const static DWORD FILE_RECV_TASK_FAILED = 6;        //文件接收失败
const static DWORD FILE_RECV_TASK_CANCEL = 7;        //文件接收任务取消

#endif

#endif //NETCOMBTRANSFER_FILETRANSFERTASKSTATE_H
