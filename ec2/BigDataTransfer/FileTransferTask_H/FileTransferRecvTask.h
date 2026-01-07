//
// Created by 王炳棋 on 2023/1/11.
//

#ifndef NETCOMBTRANSFER_FILETRANSFERRECVTASK_H
#define NETCOMBTRANSFER_FILETRANSFERRECVTASK_H

#include <fstream>
#include "FileTransferTask.h"
#include "DataAndMessage_H/FileRecvSuccess.h"
#include "DataAndMessage_H/FileRecvFailed.h"

namespace BigDataTransfer {
    class FileTransferRecvTask : public FileTransferTask {
    public:
        FileTransferRecvTask(DWORD dSrcTermId, DWORD dDestTermId, string FileAbsoluteName, shared_ptr<FileTransferData> p_data);

        ~FileTransferRecvTask() {
            pthread_mutex_destroy(&MyQueMutex_);
        }

    public:
        virtual bool OnRecvData(shared_ptr<AbstractData> pRecvData) override;

        virtual bool OnRecvMessage(shared_ptr<AbstractMessage> pRecvMessage) override;

        virtual bool SetTransferCancel() override;

        virtual bool DoTask() override;

        void CloseAndFlushFileAndCallBack();    //关闭并更新文件，同时调用回调函数通知上层

        void FlushFileAndClearCache();    //刷新文件，并清除缓存
#ifdef BIGDATATRANSFER_TEST
        DWORD32 m_d_count = 0;	//记录一共收到了多少个包
#endif

    private:
        //文件接收任务还未处理，但是已经接收了的数据
        queue<std::shared_ptr<FileTransferData> > m_p_recv_data_queue;
        // 通过 FileTransferTask 继承
        //文件接收任务已经处理，但是还没有完全写入文件的数据
        queue<std::shared_ptr<FileTransferData> > m_p_cache_data_queue;

        std::ofstream m_f_stream;

        pthread_mutex_t MyQueMutex_;    //用于队列数据的读取
    };
}

#endif //NETCOMBTRANSFER_FILETRANSFERRECVTASK_H
