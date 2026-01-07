//
// Created by 王炳棋 on 2023/1/12.
//

//本文件用作代替临时的SDKInterfaceParams头文件

#ifndef NETCOMBTRANSFER_TEMPSDKINTERFACEPARAMS_H
#define NETCOMBTRANSFER_TEMPSDKINTERFACEPARAMS_H

#include <utility>

#include "../mGlobalDef.h"

//namespace {
//    const DWORD TASK_ID_INCREMENT = 1000000;    //一个终端最多能同时传多少个文件
//}

/**
 * 发送带宽信息
 */
typedef struct FileSendBandWidth {
    FileSendBandWidth(DWORD tid, float totalRate, DWORD status) : m_term_id(tid), m_total_rate(totalRate * 8), m_status(status) {}

    const DWORD m_term_id;    //本终端终端号
    const float m_total_rate;    //本终端总的数据发送速率，单位为kbps
    const DWORD m_status;    //API可用性，为200时表示返回的值是可信的；否则表示不可用
} SendBandWidth;

/**
 * 接收带宽信息
 */
typedef struct FileRecvBandWidth {
    FileRecvBandWidth(DWORD tid, float totalRate, DWORD status) : m_term_id(tid), m_total_rate(totalRate * 8), m_status(status) {}

    const DWORD m_term_id;    //本终端终端号
    const float m_total_rate;    //本终端总的数据接收速率，单位为kbps
    const DWORD m_status;     //API可用性，为200时表示返回的值是可信的；否则表示不可用
} RecvBandWidth;

/**
 * 文件发送任务进度信息
 */
typedef struct FileTaskSendProgressInfo {
    FileTaskSendProgressInfo(DWORD task_id, DWORD d_term_id, std::string filename, float current_rate,
                             DWORD proc) : m_task_id(task_id), m_term_id(d_term_id), m_filename(std::move(filename)),
                                           m_current_rate(current_rate * 8), m_current_proc(proc) {}

    const DWORD m_task_id;    //全局唯一任务号
    const DWORD m_term_id;    //远端终端号
    const DWORD status = 1;    //为1表示终端上线，为0表示终端下线
    const std::string m_filename;    //传输文件的纯文件名
    const float m_current_rate;    //实时文件传输速率，单位为kbps
    const DWORD m_current_proc;    //实时文件发送进度，百分比为单位，区间[0,100]
    const DWORD m_send_status = 1;    //发送状态，为1表示发送进行中，为2表示发送成功，为3表示发送失败
} FileTaskSendProgressInfo;

/**
 * 文件接收任务进度信息
 */
typedef struct FileTaskRecvProgressInfo {
    FileTaskRecvProgressInfo(DWORD task_id, DWORD d_tid, std::string filename, float currentRate, DWORD currentProc) :
            m_task_id(task_id), m_term_id(d_tid), m_filename(std::move(filename)), m_current_rate(currentRate * 8), m_current_proc(currentProc) {}

    const DWORD m_task_id;    //全局唯一任务号
    const DWORD m_term_id;    //远端终端号
    const DWORD status = 1;    //为1表示终端上线，为0表示终端下线
    const std::string m_filename;    //传输文件的纯文件名
    const float m_current_rate;    //实时文件传输速率，单位为kbps
    const DWORD m_current_proc;    //实时文件发送进度，百分比为单位，区间[0,100]
    const DWORD m_recv_status = 1;    //发送状态，为1表示接收进行中，为2表示接收成功，为3表示接收失败
} FileTaskRecvProgressInfo;

/**
 * 文件发送任务状态信息
 */
typedef struct FileTaskSendStatusInfo {
    FileTaskSendStatusInfo(DWORD task_id, DWORD d_term_id, std::string filename, std::string absolute_path,
                           DWORD currentSendStatus, float current_rate, DWORD time_cost, DWORD64 file_size)
            : m_task_id(task_id), m_term_id(d_term_id), m_filename(std::move(filename)), m_file_absolute_path(std::move(absolute_path)),
              m_send_status(currentSendStatus), m_current_rate(current_rate * 8), m_time_second(time_cost),
              m_file_size(file_size) {}

    const DWORD m_task_id;    //全局唯一任务号
    const DWORD m_term_id;    //远端终端号
    const DWORD status = 1;    //为1表示终端上线，为0表示终端下线
    const std::string m_filename;    //传输文件的纯文件名
    const std::string m_file_absolute_path;    //传输文件的绝对路径
    const float m_current_rate;    //实时文件传输速率，单位为kbps
    const DWORD m_current_proc = 0;    //恒为0
    const DWORD m_send_status;    //发送状态，值的含义详见FilerTransferTaskState.h
    const DWORD m_time_second;    //耗时多少毫秒
    const DWORD64 m_file_size;    //文件大小B
} FileTaskSendStatusInfo;

/**
 * 文件接收任务状态信息
 */
typedef struct FileTaskRecvStatusInfo {
    FileTaskRecvStatusInfo(DWORD task_id, DWORD d_term_id, std::string filename, std::string absolute_path,
                           DWORD currentRecvStatus, float current_rate, DWORD time_cost, DWORD64 file_size)
            : m_task_id(task_id), m_term_id(d_term_id), m_filename(std::move(filename)), m_file_absolute_path(std::move(absolute_path)),
              m_recv_status(currentRecvStatus), m_current_rate(current_rate * 8),
              m_time_second(time_cost), m_file_size(file_size) {}
    const DWORD m_task_id;    //全局唯一任务号
    const DWORD m_term_id;    //远端终端号
    const DWORD status = 1;    //为1表示终端上线，为0表示终端下线
    const std::string m_filename;    //传输文件的纯文件名
    const std::string m_file_absolute_path;    //传输文件的绝对路径
    const float m_current_rate;    //实时文件传输速率，单位为kbps
    const DWORD m_current_proc = 0;    //恒为0
    const DWORD m_recv_status;    //状态，值的含义详见FilerTransferTaskState.h
    const DWORD m_time_second;    //耗时多少秒
    const DWORD64 m_file_size;    //文件大小B

} FileTaskRecvStatusInfo;

#endif //NETCOMBTRANSFER_TEMPSDKINTERFACEPARAMS_H
