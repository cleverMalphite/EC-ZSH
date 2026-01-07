//
// Created by 王炳棋 on 2023/1/15.
//

#ifndef NETCOMBTRANSFER_BIGDATATRANSFERMANAGER_H
#define NETCOMBTRANSFER_BIGDATATRANSFERMANAGER_H

#include <map>
#include "TempSDKInterfaceParams.h"
#include "FileTransferTask_H/FileTransferTask.h"
#include "DataAndMessage_H/FileRecvSuccess.h"
#include "DataAndMessage_H/FileRecvFailed.h"
#include "DataAndMessage_H/FileTransferData.h"
#include "DataAndMessage_H/FileSendTaskCancel.h"
#include "../IniHandle/IniHandleApi.h"


namespace BigDataTransfer {


    /**
	 * BigDataTransfer模块的管理类
	 */
    class BigDataTransferManager {
    public:
        BigDataTransferManager();

        ~BigDataTransferManager() {
            pthread_mutex_destroy(&Mutex_);
        }

    public:


        void DispatchFileTransferData(DWORD dTermId, const shared_ptr<FileTransferData> &pData);

        void DispatchFileRecvSuccessMessage(DWORD dTermId, shared_ptr<FileRecvSuccess> pMessage);

        void DispatchFileSendTaskCancelMessage(DWORD dTermId, shared_ptr<FileSendTaskCancel> pMessage);

        void DispatchFileRecvFailedMessage(DWORD dTermId, const shared_ptr<FileRecvFailed> &pMessage);

        bool Create_BigDataTransferTask(DWORD dRemoteId, const std::string &s_file_name,
                                        const std::string &s_file_absolute_name, DWORD &TaskID);

        bool Create_BigDataTransferTask(DWORD dRemoteId, const std::string &s_file_name,
                                        const std::string &s_file_absolute_name, DWORD &TaskID, bool is_RBUDP);

        //循环遍历每一个文件发送或者接收任务
        bool DoTask();  //返回true表示没有任务需要处理，返回false表示有任务需要处理

        bool SetFileSendTaskCancel(DWORD task_id);  //取消某个文件发送任务
        //统计文件发送与接收总带宽
        std::shared_ptr<FileSendBandWidth> GetAllFileSendBandWidth();

        std::shared_ptr<FileRecvBandWidth> GetAllFileRecvBandWidth();

        void changeFunc(bool isRBUDP) { m_isRBUDP = isRBUDP; }



    private:
        //接收文件的目录
        const string m_file_recv_dir = GetStringValueKeyIni("BigDataTransfer", "recv_dir", "");    
         //本终端的终端号
        const DWORD m_term_id = GetIntegerKeyIni("Main", "DeviceID", 100);                
        //两个map对应的关键字都是任务ID
        std::map<DWORD, shared_ptr<FileTransferTask>> m_recv_task_map;    //文件接收任务映射map
        std::map<DWORD, shared_ptr<FileTransferTask>> m_send_task_map;    //文件发送任务映射map
        std::vector<DWORD> m_task_id_used;
        bool m_isRBUDP;   //用于判断当前使用的是哪一种文件发送方式

        pthread_mutex_t Mutex_;
    private:
        DWORD GenerateTaskId();    //创建全局唯一任务ID

        bool FindSendTaskById(DWORD dTaskId);    //如果有ID为dTaskId的文件发送任务，那么返回true，否则返回false
    };
}

#endif //NETCOMBTRANSFER_BIGDATATRANSFERMANAGER_H
