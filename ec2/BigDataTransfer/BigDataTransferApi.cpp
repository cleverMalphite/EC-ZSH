//
// Created by 王炳棋 on 2023/1/29.
//
#include "BigDataTransferApi.h"
#include <thread>

#include "infoHub/infoHub.h"
#include "infoHub/rateStatistic.h"
#include "infoHub/rateStatisticTable.h"
#include "infoHub/progressStatisticTable.h"


//----------------------bigdatatransfer(信息仓库)初始化-------------------------
/*bigdatatransfer模块的信息需求清单
  1. 该层总的传输速率：
        rx_rate,                            (value名, 类型为i, 单位)
        tx_rate                             (value名, 类型为i, 单位)
  2. 各个文件的传输速率:
        map<fid, rate> rx_fid_rate_map,     (table名, 类型i2i, 单位)
        map<fid, rate> tx_fid_rate_map      (table名, 类型i2i, 单位)

  * key名，即为value名，table名
  * 上述table的field名即为fid
  * module名为bigdatatransfer
*/

std::shared_ptr<ec2::infoHub>\
        _bigdatatransfer_infohub_instance = \
            ec2::infoHub::getInstance();

ec2::rateStatistic      bdt_rx_rate_stat;
ec2::rateStatistic      bdt_tx_rate_stat;

ec2::rateStatisticTable rx_fid_rate_sttable;
ec2::rateStatisticTable tx_fid_rate_sttable;

//uint64_t send_file_id_index;
//uint64_t recv_file_id_index;


// fid_recvfilename_map
// fid_sendfilename_map

// rx_fid_speedkBps_sttable;
// tx_fid_speedkBps_sttable;

// rx_current_speedkBps_stat;

/*
 * taskId 101*1000000+000001
 * 本端终端号是全局唯一的，后面加上一个循环的文件唯一ID（1-1000000）
 * 限制了终端总数小于1000
*/
ec2::progressStatisticTable rcv_fid_progress_sttable;
ec2::progressStatisticTable snd_fid_progress_sttable;

const DWORD TASK_ID_INCREMENT = 1000000;    //一个终端最多能同时传多少个文件

namespace {
    const DWORD MAX_CALLBACK_NUM = 10;
    DWORD RegisterFileSendTaskProgressCallBack_Number = 0;    //相关回调函数已被注册的数目
    DWORD RegisterFileRecvTaskProgressCallBack_Number = 0;    //相关回调函数已被注册的数目
    DWORD RegisterFileSendTaskStatusCallBack_Number = 0;    //相关回调函数已被注册的数目
    DWORD RegisterFileRecvTaskStatusCallBack_Number = 0;    //相关回调函数已被注册的数目
}

pRegisterFileSendTaskProgressCallBack g_pRegisterFileSendTaskProgressCallBack[MAX_CALLBACK_NUM];
pRegisterFileRecvTaskProgressCallBack g_pRegisterFileRecvTaskProgressCallBack[MAX_CALLBACK_NUM];
pRegisterFileSendTaskStatusCallBack g_pRegisterFileSendTaskStatusCallBack[MAX_CALLBACK_NUM];
pRegisterFileRecvTaskStatusCallBack g_pRegisterFileRecvTaskStatusCallBack[MAX_CALLBACK_NUM];

namespace BigDataTransfer {


    std::shared_ptr<BigDataTransferManager> gBigDataTransferManager = nullptr;
    //bool g_bStop = false;
    std::atomic_flag ThreadRunningFlag = ATOMIC_FLAG_INIT;
    /*const DWORD BIGDATA_TRANSFER_ID_TIMER = 2002;*/
    /*HANDLE g_bigdata_transfer_event = CreateEvent(NULL, 1, 1, NULL);*/

    void BigDataTransferMessageDealFunc() {
        bool dResult = false;
        while (ThreadRunningFlag.test_and_set()) {
            if (gBigDataTransferManager) {
//                printf("BigDataAPI:MessagDealFunc.1\n");
                dResult = gBigDataTransferManager->DoTask();
                //printf("running...\n");
                if (!dResult) {
                    //如果没有任务需要处理，那么设置信号量要等待
                    //ResetEvent(g_bigdata_transfer_event);
                    //WaitForSingleObject(g_bigdata_transfer_event, 2000);
                    //std::this_thread::sleep_for(std::chrono::microseconds(1));
                    usleep(100*1000);
                }
            }
        }
        //printf("[BigDataTransfer]::工作线程终止\n");
        ThreadRunningFlag.clear();
    }

    /**
	 * 调用相关别的模块的全部数据回调函数
	 */
    void CallAll_RegisterFileSendTaskProgressCallBack(const shared_ptr<FileTaskSendProgressInfo>& info) {
        for (DWORD i = 0; i < RegisterFileSendTaskProgressCallBack_Number; i++) {
            if (nullptr != g_pRegisterFileSendTaskProgressCallBack[i]) {
                g_pRegisterFileSendTaskProgressCallBack[i](info);
            }
        }
    }

    /**
     * 调用相关别的模块的全部数据回调函数
     */
    void CallAll_RegisterFileRecvTaskProgressCallBack(const shared_ptr<FileTaskRecvProgressInfo>& info) {
        for (DWORD i = 0; i < RegisterFileRecvTaskProgressCallBack_Number; i++) {
            if (nullptr != g_pRegisterFileRecvTaskProgressCallBack[i]) {
                g_pRegisterFileRecvTaskProgressCallBack[i](info);
            }
        }
    }

    /**
     * 调用相关别的模块的全部数据回调函数
     */
    void CallAll_RegisterFileSendTaskStatusCallBack(const shared_ptr<FileTaskSendStatusInfo>& info) {
        for (DWORD i = 0; i < RegisterFileSendTaskStatusCallBack_Number; i++) {
            if (nullptr != g_pRegisterFileSendTaskStatusCallBack[i]) {
                g_pRegisterFileSendTaskStatusCallBack[i](info);
            }
        }
    }

    /**
     * 调用相关别的模块的全部数据回调函数
     */
    void CallAll_RegisterFileRecvTaskStatusCallBack(const shared_ptr<FileTaskRecvStatusInfo>& info) {
        for (DWORD i = 0; i < RegisterFileRecvTaskStatusCallBack_Number; i++) {
            if (nullptr != g_pRegisterFileRecvTaskStatusCallBack[i]) {
                g_pRegisterFileRecvTaskStatusCallBack[i](info);
            }
        }
    }

}


using BigDataTransfer::BigDataTransferManager;
using BigDataTransfer::gBigDataTransferManager;

bool InitBigDataTransfer() {
    //初始化模块内的日志组件
    if (nullptr == gBigDataTransferManager) {
        gBigDataTransferManager = std::make_shared<BigDataTransferManager>();
    }

    //开启另一个线程来处理循环消息
    BigDataTransfer::ThreadRunningFlag.test_and_set();

    std::thread work_thread(BigDataTransfer::BigDataTransferMessageDealFunc);
    work_thread.detach();


    bdt_rx_rate_stat.init("bigdatatransfer","rx_rate_stat");
    bdt_tx_rate_stat.init("bigdatatransfer","tx_rate_stat");
    
    rx_fid_rate_sttable.init("bigdatatransfer", "rx_fid_rate_sttable");
    tx_fid_rate_sttable.init("bigdatatransfer", "tx_fid_rate_sttable");
    
    rcv_fid_progress_sttable.init("bigdatatransfer", "rcv_fid_progress_sttable");
    snd_fid_progress_sttable.init("bigdatatransfer", "snd_fid_progress_sttable");

    bdt_rx_rate_stat.begin();
    bdt_tx_rate_stat.begin();
    
    //rx_fid_rate_sttable.begin();
    //tx_fid_rate_sttable.begin();

    return true;
}

bool UninitBigDataTransfer() {
    BigDataTransfer::ThreadRunningFlag.clear();
    usleep(1000 * 100);
    gBigDataTransferManager = nullptr;
    return true;
}

bool Create_BigDataTransferTask(DWORD dRemoteId, const std::string& s_file_name, const std::string& s_file_absolute_name, DWORD&TaskID) {
    if (gBigDataTransferManager) {
        return gBigDataTransferManager->Create_BigDataTransferTask(dRemoteId, s_file_name, s_file_absolute_name, TaskID);
    }
    return false;
}

std::shared_ptr<FileSendBandWidth> BigDataTransfer_GetAllFileSendBandWidth() {
    return gBigDataTransferManager->GetAllFileSendBandWidth();
}

std::shared_ptr<FileRecvBandWidth> BigDataTransfer_GetAllFileRecvBandWidth() {
    return gBigDataTransferManager->GetAllFileRecvBandWidth();
}

bool SetFileSendTaskCancel(DWORD task_id) {
    return gBigDataTransferManager->SetFileSendTaskCancel(task_id);
}

bool RegisterFileSendTaskProgressCallBack(pRegisterFileSendTaskProgressCallBack pFunc) {
    printf("BigDataAPI：RegisterFileSendTaskProgressCallBack。1开始注册回调\n");
    if (MAX_CALLBACK_NUM <= RegisterFileSendTaskProgressCallBack_Number) { return false; }
    else if (nullptr == pFunc) { return false; }
    else {
        printf("BigDataAPI：RegisterFileSendTaskProgressCallBack。2注册进去了\n");
        g_pRegisterFileSendTaskProgressCallBack[RegisterFileSendTaskProgressCallBack_Number] = pFunc;
        RegisterFileSendTaskProgressCallBack_Number++;
        return true;
    }
}

bool RegisterFileRecvTaskProgressCallBack(pRegisterFileRecvTaskProgressCallBack pFunc) {
    if (MAX_CALLBACK_NUM <= RegisterFileRecvTaskProgressCallBack_Number) { return false; }
    else if (nullptr == pFunc) { return false; }
    else {
        g_pRegisterFileRecvTaskProgressCallBack[RegisterFileRecvTaskProgressCallBack_Number] = pFunc;
        RegisterFileRecvTaskProgressCallBack_Number++;
        return true;
    }
}

bool RegisterFileSendTaskStatusCallBack(pRegisterFileSendTaskStatusCallBack pFunc) {
    if (MAX_CALLBACK_NUM <= RegisterFileSendTaskStatusCallBack_Number) { return false; }
    else if (nullptr == pFunc) { return false; }
    else {
        g_pRegisterFileSendTaskStatusCallBack[RegisterFileSendTaskStatusCallBack_Number] = pFunc;
        RegisterFileSendTaskStatusCallBack_Number++;
        return true;
    }
}

bool RegisterFileRecvTaskStatusCallBack(pRegisterFileRecvTaskStatusCallBack pFunc) {
    if (MAX_CALLBACK_NUM <= RegisterFileRecvTaskStatusCallBack_Number) { return false; }
    else if (nullptr == pFunc) { return false; }
    else {
        g_pRegisterFileRecvTaskStatusCallBack[RegisterFileRecvTaskStatusCallBack_Number] = pFunc;
        RegisterFileRecvTaskStatusCallBack_Number++;
        return true;
    }
}

bool
Create_BigDataTransferTask(DWORD RemoteTID, const std::string &s_file_name, const std::string &s_file_absolute_name,
                           DWORD &TaskID, bool is_RBUDP) {
    if (gBigDataTransferManager) {
        return gBigDataTransferManager->Create_BigDataTransferTask(RemoteTID, s_file_name, s_file_absolute_name, TaskID,is_RBUDP);
    }
    return false;
}

