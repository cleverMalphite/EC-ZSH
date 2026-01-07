#ifndef __EC2_H__
#define __EC2_H__


#include <iostream>
#include "GlobalMessage.h"
#include "epollComm/EpollCommApi.h"
#include "RBUDP/RBUDPApi.h"
#include "MRUDP/MRUDPApi.h"
#include "Util/LockGuard.h"
#include "NetCombTransferApi.h"
#include "BigDataTransfer/BigDataTransferApi.h"
#include "CNetTerminalManager.h"
#include "SpeedControl/SpeedControlManager.h"
#include "Term2TermQos/TermToTermQosApi.h"
#include "time.h"

#include "public/_public.h"


/*--------------------------初始化操作---------------------------*/
//系统启动
void System_start(const char *config_path, bool QosOpen = true, bool Is_RBUDP = false) ;

//系统关闭
void System_shutdown(bool QosOpen = true, bool Is_RBUDP = false) ;

//接收方创建通道
CreateTcpServerChannel(starg.localport, starg.localip, 3020, 100, true);
CreateDtuTcpServerChannel(starg.localport, starg.localip, 3020, 100);

//发送方创建通道
CreateTcpClientChannel(starg.localport, starg.remoteport, starg.localip, starg.remoteip, 3200);
CreateDtuTcpClientChannel(starg.localport, starg.remoteport, starg.localip, starg.remoteip, 3200);

/*--------------------------文件发送操作---------------------------*/

//单文件发送
Create_BigDataTransferTask(/*102*/starg.remotetid, starg.sendfilename, starg.sendfilepath, TaskID, _is_rbudp);


//批量文件发送
Create_BulkDataTransferTask(/*102*/starg.remotetid, starg.sendfilepath, _is_rbudp);


/*--------------------------信息获取操作---------------------------*/

//获取发送文件信息（任务id，文件名，发送路径，总大小，已发送大小，发送速率）
getSndTaskInfo( task_id, file_name, send_path, total_size, send_size, send_rate);

//获取接收文件速率（任务id，文件名，接收路径，总大小，已接收大小，接收速率）
getRcvTaskInfo( task_id, file_name, recv_path, total_size, recv_size, recv_rate);






//系统启动
void System_start(const char *config_path, bool QosOpen , bool Is_RBUDP) 
{
    InitIniHandler(config_path);
    DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
    // ... ...
    Init_NetCombTransfer(TermId);
    InitSpeedControl(TermId);
    //InitTermToTermQos(TermId);
    sleep(1);
    if (QosOpen) {
        InitTermToTermQos(TermId);
    }
    if (Is_RBUDP) {
        InitRBUDP("");
        std::cout << "此时可靠传输模块为：RBUDP" << endl;
    } else {
        InitMRUDP("");
        std::cout << "此时可靠传输模块为：MRUDP" << endl;
    }

    InitBigDataTransfer();
    //SystemControlInit();
    //printf("[SystemTest]::Terminal %d had inited\n", TermId);
}

//系统关闭
void System_shutdown(bool QosOpen, bool Is_RBUDP) {
    DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
    UninitBigDataTransfer();
    if (Is_RBUDP) {
        UnInitRBUDP();
    } else {
        UnInitMRUDP();
    }
    if (QosOpen) {
        UninitTermToTermQos();
    }
    UninitSpeedControl();
    UnInit_NetCombTransfer();
    // ... ...
    UninitIniHandle();
    //printf("[SystemTest]::Terminal %d had shut down\n", TermId);
}

#endif // __EC2_H__
