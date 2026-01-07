
// Created by 王炳棋 on 2023/1/11.
//



#ifndef NETCOMBTRANSFER_FILETRANSFERSENDTASK_H
#define NETCOMBTRANSFER_FILETRANSFERSENDTASK_H

#include <fstream>
#include "../../Util/LockGuard.h"
#include "FileTransferTask_H//FileTransferTask.h"
#include "DataAndMessage_H/FileRecvSuccess.h"
#include "DataAndMessage_H/FileRecvFailed.h"

namespace {

}

namespace BigDataTransfer {

    class FileSendTransferByteData {
    public:
        FileSendTransferByteData(DWORD dLength, std::shared_ptr<BYTE> buffer, DWORD d_real_length = FileTransferTask::SINGLE_BUFFER_SIZE) {
            m_d_length = dLength;
            m_buffer = buffer;
            m_d_read_data_length = d_real_length;
        }

        DWORD GetLength() const { return m_d_length; }

        std::shared_ptr<BYTE> GetBuffer() { return m_buffer; }

        DWORD GetRealDataLength() const { return m_d_read_data_length; }

    private:
        DWORD m_d_length = 0;
        DWORD m_d_read_data_length;    //数据中纯文件数据的长度
        std::shared_ptr<BYTE> m_buffer;
    };
}

namespace BigDataTransfer {

    class FileTransferSendTask : public FileTransferTask {
    public:
        FileTransferSendTask(DWORD dSrcTermId, DWORD dRemoteId, DWORD dTaskId, string file_name, string file_absolute_name);

        FileTransferSendTask(DWORD dSrcTermId, DWORD dRemoteId, DWORD dTaskId, string file_name, string file_absolute_name,bool is_RBUDP);

        ~FileTransferSendTask() override {
            if (m_p_file.is_open()) {
                m_p_file.close();
            }
            pthread_mutex_destroy(&_transferStateMutex);
        }

        // 通过 FileTransferTask 继承
        bool OnRecvData(std::shared_ptr<AbstractData> pRecvData) override;

        bool OnRecvMessage(std::shared_ptr<AbstractMessage> pRecvMessage) override;

        bool DoTask() override;

        bool SetTransferCancel() override;

    private:
        //存储还未成功进行有序可靠发送的数据的字节序列集合
        std::queue<std::shared_ptr<FileSendTransferByteData> > m_buffer_queue;    //存储还未成功发送的字节序列

        bool m_b_is_file_read_all;    //判断文件是否已经被读取完毕

        std::ifstream m_p_file;        //与此任务关联的文件指针

        pthread_mutex_t _transferStateMutex; //文件状态需要同步,现在采用锁做同步

        //DWORD m_file_send_length;	//已经发送的数据长度（字节），注意包括重传的字节
    private:

        void SendData();

        void DoRecvFileRecvFailed(std::shared_ptr<FileRecvFailed> pRecvMessage);

        void DoRecvFileSendFailed();

        void DoRecvFileRecvSuccess(const std::shared_ptr<FileRecvSuccess> &pRecvMessage);

    };
}
#endif //NETCOMBTRANSFER_FILETRANSFERSENDTASK_H
