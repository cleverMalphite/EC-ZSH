//
// Created by 王炳棋 on 2023/1/15.
//

#include "../Util/LockGuard.h"
#include "../MRUDP/MRUDPApi.h"
#include "../RBUDP/RBUDPApi.h"
#include "BigDataTransferManager.h"
#include "FileTransferTask_H/FileTransferRecvTask.h"
#include "FileTransferTask_H/FileTransferSendTask.h"

//namespace {
//    const DWORD TASK_ID_INCREMENT = 1000000;    //一个终端最多能同时传多少个文件
//}

extern const DWORD TASK_ID_INCREMENT = 1000000;    //一个终端最多能同时传多少个文件

namespace BigDataTransfer {

    extern std::shared_ptr<BigDataTransferManager> gBigDataTransferManager;

    //回调函数：对应MRUDP中定义的数据回调接口PGETDATAFROMMRUDPFORBIGDATATRANSFERCALLBACK
    void OnRecvMRUDPData(DWORD dRemoteId, 
                         const std::shared_ptr<BYTE> &pDataBytes, 
                         DWORD dRecvDataByteLength) 
    {
        if (nullptr == gBigDataTransferManager) {
            return;
        }

        shared_ptr<BYTE> pRecvDataBytes(new BYTE[dRecvDataByteLength], releaseArrays<BYTE>);
        memcpy(pRecvDataBytes.get(), pDataBytes.get(), dRecvDataByteLength);
        /**
         * 值得注意的是，这里的信令或者数据的判别处理需要按照这些数据或者信令的实际接收频率的估计值依次进行
         */

        if (FileTransferData::IsEncodeInstanceOf(pRecvDataBytes, dRecvDataByteLength)) {
            printf("BDT => is File Transfer Data!\n");
            std::shared_ptr<FileTransferData> p_transfer_data = std::make_shared<FileTransferData>();
            p_transfer_data->Decode(pRecvDataBytes, dRecvDataByteLength);
#ifdef BIGDATATRANSFER_TEST
            static DWORD32 d_recv_confirmed_packet_number = 0;
            d_recv_confirmed_packet_number++;
            if (d_recv_confirmed_packet_number != p_transfer_data->GetCurrentPacketNumberHasTransfered())
            {
                d_recv_confirmed_packet_number = d_recv_confirmed_packet_number;
            }
#endif // BIGDATATRANSFER_TEST

            gBigDataTransferManager->DispatchFileTransferData(dRemoteId, p_transfer_data);
            return;
        }

        if (FileRecvSuccess::IsEncodeInstanceOf(pRecvDataBytes, dRecvDataByteLength)) {
            printf("is File Recv Success!\n");
            std::shared_ptr<FileRecvSuccess> p_message = std::make_shared<FileRecvSuccess>();
            p_message->Decode(pRecvDataBytes, dRecvDataByteLength);
            gBigDataTransferManager->DispatchFileRecvSuccessMessage(dRemoteId, p_message);
            return;
        }

        if (FileSendTaskCancel::IsEncodeInstanceOf(pRecvDataBytes, dRecvDataByteLength)) {
            printf("is File Task Cancel!\n");
            shared_ptr<FileSendTaskCancel> p_message = std::make_shared<FileSendTaskCancel>();
            p_message->Decode(pRecvDataBytes, dRecvDataByteLength);
            gBigDataTransferManager->DispatchFileSendTaskCancelMessage(dRemoteId, p_message);
            return;
        }

        if (FileRecvFailed::IsEncodeInstanceOf(pRecvDataBytes, dRecvDataByteLength)) {
            printf("is File Recv Failed!\n");
            shared_ptr<FileRecvFailed> p_message = std::make_shared<FileRecvFailed>();
            p_message->Decode(pRecvDataBytes, dRecvDataByteLength);
            gBigDataTransferManager->DispatchFileRecvFailedMessage(dRemoteId, p_message);
            return;
        }
        printf("BDT => Not Instance of BDT Data!\n");
    }

    //回调函数：对应MRUDP中定义的数据回调接口PGETDATAFROMRBUDPFORBIGDATATRANSFERCALLBACK
    void OnRecvRBUDPData(unsigned long int dRemoteId, std::shared_ptr<unsigned char> pDataBytes, unsigned long int dRecvDataByteLength) {
        if (nullptr == gBigDataTransferManager) {
            return;
        }

        shared_ptr<BYTE> pRecvDataBytes(new BYTE[dRecvDataByteLength], releaseArrays<BYTE>);
        memcpy(pRecvDataBytes.get(), pDataBytes.get(), dRecvDataByteLength);
        /**
         * 值得注意的是，这里的信令或者数据的判别处理需要按照这些数据或者信令的实际接收频率的估计值依次进行
         */

        if (FileTransferData::IsEncodeInstanceOf(pRecvDataBytes, dRecvDataByteLength)) {
            std::shared_ptr<FileTransferData> p_transfer_data = std::make_shared<FileTransferData>();
            p_transfer_data->Decode(pRecvDataBytes, dRecvDataByteLength);
#ifdef BIGDATATRANSFER_TEST
            static DWORD32 d_recv_confirmed_packet_number = 0;
            d_recv_confirmed_packet_number++;
            if (d_recv_confirmed_packet_number != p_transfer_data->GetCurrentPacketNumberHasTransfered())
            {
                d_recv_confirmed_packet_number = d_recv_confirmed_packet_number;
            }
#endif // BIGDATATRANSFER_TEST

            gBigDataTransferManager->DispatchFileTransferData(dRemoteId, p_transfer_data);
            return;
        }

        if (FileRecvSuccess::IsEncodeInstanceOf(pRecvDataBytes, dRecvDataByteLength)) {
            std::shared_ptr<FileRecvSuccess> p_message = std::make_shared<FileRecvSuccess>();
            p_message->Decode(pRecvDataBytes, dRecvDataByteLength);
            gBigDataTransferManager->DispatchFileRecvSuccessMessage(dRemoteId, p_message);
            return;
        }

        if (FileSendTaskCancel::IsEncodeInstanceOf(pRecvDataBytes, dRecvDataByteLength)) {
            shared_ptr<FileSendTaskCancel> p_message = std::make_shared<FileSendTaskCancel>();
            p_message->Decode(pRecvDataBytes, dRecvDataByteLength);
            gBigDataTransferManager->DispatchFileSendTaskCancelMessage(dRemoteId, p_message);
            return;
        }

        if (FileRecvFailed::IsEncodeInstanceOf(pRecvDataBytes, dRecvDataByteLength)) {
            shared_ptr<FileRecvFailed> p_message = std::make_shared<FileRecvFailed>();
            p_message->Decode(pRecvDataBytes, dRecvDataByteLength);
            gBigDataTransferManager->DispatchFileRecvFailedMessage(dRemoteId, p_message);
            return;
        }
    }


    void OnRecvFuncSelect(bool isRBUDP){
        gBigDataTransferManager->changeFunc(isRBUDP);
    }
    BigDataTransferManager::BigDataTransferManager() {

        //Register_isRBUDP_CallBack(OnRecvFuncSelect);
        RegisterRBUDPFunc(OnRecvRBUDPData);

        RegisterMRUDPFunc(OnRecvMRUDPData);
        pthread_mutex_init(&Mutex_, nullptr);
    }

    DWORD BigDataTransferManager::GenerateTaskId() {
        /**
		 * 思路：
		 * 本端终端号是全局唯一的，后面加上一个循环的文件唯一ID（1-1000000）
		 * 限制了终端总数小于1000
		 */
		 //ps:线程不安全的
         //存在爆内存的风险
        if (TASK_ID_INCREMENT <= m_send_task_map.size()) {
            return 0;    //表示没有多余的TASK ID
        }

        const static DWORD d_begin_global_task_id = m_term_id * 1000000;
        static DWORD d_task_id_increment = 0;
        DWORD d_result = d_begin_global_task_id + d_task_id_increment;


        while (FindSendTaskById(d_result)) {
            d_task_id_increment = (d_task_id_increment + 1) % TASK_ID_INCREMENT;
            d_result = d_begin_global_task_id + d_task_id_increment;
        }
        m_task_id_used.push_back(d_result);
        return d_result;
    }

    bool BigDataTransferManager::FindSendTaskById(DWORD dTaskId) {
        auto iter = m_task_id_used.begin();
        for (; iter != m_task_id_used.end(); iter++) {
            if (*iter == dTaskId) {
                return true;
            }
        }
        return false;
    }

    void BigDataTransferManager::DispatchFileTransferData(DWORD dTermId, const std::shared_ptr<FileTransferData> &pData) {
        //文件传输数据要给也是给文件接收任务，所以只需要查找是否有对应的文件接收任务即可
        MutexLockGuard BigDataTransferManagerGlobalLock(Mutex_);
        if (!m_send_task_map.empty()) {
            return;
        }
        DWORD d_recv_task_id = pData->GetFileTransferTaskId();
        auto iter = m_recv_task_map.find(d_recv_task_id);
        if (iter != m_recv_task_map.end()) {
            //说明已经有对应的文件接收任务了，把数据传给对应的数据接收任务即可
            iter->second->OnRecvData(pData);
        } else {
            if (1 == pData->GetCurrentPacketNumberHasTransfered()) {
                //创建新的文件接收任务
                string s_absolute_name = m_file_recv_dir + pData->GetFileName();
                std::shared_ptr<FileTransferRecvTask> p_recv_task = std::make_shared<FileTransferRecvTask>(dTermId, m_term_id,
                                                                                                           s_absolute_name,
                                                                                                           pData);
                //将新的文件接收任务放入文件接收任务映射中去
                m_recv_task_map.insert(make_pair(pData->GetFileTransferTaskId(), p_recv_task));
            }
        }
    }

    void BigDataTransferManager::DispatchFileRecvSuccessMessage(DWORD dTermId, std::shared_ptr<FileRecvSuccess> pMessage) {
        //看是否是对应的文件发送任务
        DWORD d_send_task_id = pMessage->GetFileTransferTaskId();
        auto iter = m_send_task_map.find(d_send_task_id);
        if (iter != m_send_task_map.end()) {
            //更新文件发送任务
            iter->second->OnRecvMessage(pMessage);
        }
    }

    //TODO ADD 未开发
    void BigDataTransferManager::DispatchFileSendTaskCancelMessage(DWORD dTermId, std::shared_ptr<FileSendTaskCancel> pMessage) {
        //看是否是对应的文件
    }

    void BigDataTransferManager::DispatchFileRecvFailedMessage(DWORD dTermId, const std::shared_ptr<FileRecvFailed> &pMessage) {
        //看是否是对应的文件发送任务
        //Mark,回调函数和处理线程操作m_send_task_map之间存在同步;
        DWORD d_send_task_id = pMessage->GetFileTransferTaskId();
        auto iter = m_send_task_map.find(d_send_task_id);
        if (iter != m_send_task_map.end()) {
            //更新文件发送任务
            iter->second->OnRecvMessage(pMessage);
        }
    }

    bool BigDataTransferManager::Create_BigDataTransferTask(DWORD dRemoteId, const std::string &s_file_name,
                                                            const std::string &s_file_absolute_name, DWORD &TaskID) {
        //为什么在下边，调用size()时不加锁？
        //size()不加锁，有可能导致实际并发传输数目不符合设定数目
        pthread_mutex_lock(&Mutex_);
        //如果我当前有接收任务正在进行，就不能创建发送任务;
        if (!m_recv_task_map.empty()) {
            pthread_mutex_unlock(&Mutex_);
            return false;
        }
        pthread_mutex_unlock(&Mutex_);
        const static DWORD invalid_task_id = 0;
        TaskID = GenerateTaskId();
        if (invalid_task_id == TaskID) {
            return false;
        }
        //s_file_name = std::to_string(d_task_id) + s_file_name;	//相当于重命名了
        if (m_send_task_map.size() <= 10) {
            std::shared_ptr<FileTransferSendTask> p_send_task = std::make_shared<FileTransferSendTask>(m_term_id, dRemoteId, TaskID,
                                                                                                       s_file_name, s_file_absolute_name);
            m_send_task_map.insert(make_pair(TaskID, p_send_task));
            //printf("[BigDataTransferDebug]::Success, m_send_task_map size is %lu\n", m_send_task_map.size());
            return true;
        }
        //printf("[BigDataTransferDebug]::False, m_send_task_map size is %lu\n", m_send_task_map.size());
        return false;
    }

    bool BigDataTransferManager::DoTask() {
//        printf("BigManager:Dotask.1\n");

        if (m_send_task_map.size() == 0 && m_recv_task_map.size() == 0) {
            //只有文件发送任务与文件接收任务都不存在时，才需要返回false，表示任务空闲
            return false;
        }
        auto iter_send = m_send_task_map.begin();
        std::shared_ptr<FileTransferTask> p_send_task;
        for (; iter_send != m_send_task_map.end();) {
//            printf("BigManager:Dotask.2\n");
            p_send_task = iter_send->second;
            if (p_send_task->DoTask()) {
                //删除节点，同时调用相关的数据回调函数
                //printf("[BigDataTransferDebug]::DoSendTask Finish, before SendTask Size is %lu\n", m_send_task_map.size());
                m_send_task_map.erase(iter_send++);
            } else {
                iter_send++;
            }
        }
        //Mark:这里的代码只有把所有的数据发送任务完成后，才可以执行文件接受任务，存在运行隐患。
        //更新每一个文件发送任务和文件接收任务的状态，并删除一些已经完成的元素
        //TODO ADD 注意现在还没有编写与回调函数有关的代码
        auto iter_recv = m_recv_task_map.begin();
        std::shared_ptr<FileTransferTask> p_recv_task;
        for (; iter_recv != m_recv_task_map.end();) {
//            printf("BigManager:Dotask.3\n");
            p_recv_task = iter_recv->second;
            if (p_recv_task->DoTask()) {
                //printf("[BigDataTransferDebug]::DoRecvTask Finish, before RecvTask Size is %lu\n", m_recv_task_map.size());
                m_recv_task_map.erase(iter_recv++);
            } else {
                iter_recv++;
            }
        }
        return true;
    }

    std::shared_ptr<FileSendBandWidth> BigDataTransferManager::GetAllFileSendBandWidth() {
        DWORD total_rate_kbps = 0;
        auto iter_send = m_send_task_map.begin();
        std::shared_ptr<FileTransferTask> p_send_task;
        for (; iter_send != m_send_task_map.end();) {
            p_send_task = iter_send->second;
            total_rate_kbps += p_send_task->GetTransferSpeedKbps();
            iter_send++;
        }

        shared_ptr<FileSendBandWidth> file_send_bandwidth = std::make_shared<FileSendBandWidth>
                (m_term_id, total_rate_kbps, 200);
        return file_send_bandwidth;
    }

    bool BigDataTransferManager::SetFileSendTaskCancel(DWORD task_id) {
        auto iter_send = m_send_task_map.begin();
        if (m_send_task_map.find(task_id) != m_send_task_map.end()) {
            auto p_send_task = m_send_task_map.find(task_id);
            auto second=p_send_task->second;
            p_send_task->second->SetTransferCancel();
        }

        return true;
    }

    std::shared_ptr<FileRecvBandWidth> BigDataTransferManager::GetAllFileRecvBandWidth() {
        DWORD total_rate_kbps = 0;
        auto iter_send = m_recv_task_map.begin();
        std::shared_ptr<FileTransferTask> p_recv_task;
        for (; iter_send != m_recv_task_map.end();) {
            p_recv_task = iter_send->second;
            total_rate_kbps += p_recv_task->GetTransferSpeedKbps();
            iter_send++;
        }

        shared_ptr<FileRecvBandWidth> file_recv_bandwidth = std::make_shared<FileRecvBandWidth>
                (m_term_id, total_rate_kbps, 200);
        return file_recv_bandwidth;
    }

    bool BigDataTransferManager::Create_BigDataTransferTask(DWORD dRemoteId, const string &s_file_name,
                                                            const string &s_file_absolute_name, DWORD &TaskID,
                                                            bool is_RBUDP) {
        //为什么在下边，调用size()时不加锁？
        //size()不加锁，有可能导致实际并发传输数目不符合设定数目
        pthread_mutex_lock(&Mutex_);
        //如果我当前有接收任务正在进行，就不能创建发送任务;
        if (!m_recv_task_map.empty()) {
            pthread_mutex_unlock(&Mutex_);
            return false;
        }
        pthread_mutex_unlock(&Mutex_);
        const static DWORD invalid_task_id = 0;
        TaskID = GenerateTaskId();
        if (invalid_task_id == TaskID) {
            return false;
        }
        //s_file_name = std::to_string(d_task_id) + s_file_name;	//相当于重命名了
        is_RBUDP=false;
        if (m_send_task_map.size() <= 10) {
            std::shared_ptr<FileTransferSendTask> p_send_task = std::make_shared<FileTransferSendTask>(m_term_id, dRemoteId, TaskID,
                                                                                                       s_file_name, s_file_absolute_name,is_RBUDP);
            m_send_task_map.insert(make_pair(TaskID, p_send_task));
            //printf("[BigDataTransferDebug]::Success, m_send_task_map size is %lu\n", m_send_task_map.size());
            return true;
        }
        //printf("[BigDataTransferDebug]::False, m_send_task_map size is %lu\n", m_send_task_map.size());
        return false;
    }

}
