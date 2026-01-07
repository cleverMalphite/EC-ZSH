//
// Created by 王炳棋 on 2023/1/12.
//
#include "FileTransferTask_H/FileTransferSendTask.h"
#include "../MRUDP/MRUDPApi.h"
#include "../RBUDP/RBUDPApi.h"
#include "TempSDKInterfaceParams.h"

#include "infoHub/infoHub.h"
#include "infoHub/rateStatistic.h"
#include "infoHub/rateStatisticTable.h"
#include "infoHub/progressStatisticTable.h"

extern std::shared_ptr<ec2::infoHub>\
        _bigdatatransfer_infohub_instance ;

extern ec2::rateStatistic      bdt_tx_rate_stat;
extern ec2::rateStatisticTable tx_fid_rate_sttable;

extern ec2::progressStatisticTable snd_fid_progress_sttable;

extern const DWORD TASK_ID_INCREMENT ;    //一个终端最多能同时传多少个文件


namespace BigDataTransfer {

    //extern HANDLE g_bigdata_transfer_event;	//全局内核对象，防止CPU空转
    extern void CallAll_RegisterFileSendTaskStatusCallBack(const shared_ptr<FileTaskSendStatusInfo> &info);

    extern void CallAll_RegisterFileSendTaskProgressCallBack(const shared_ptr<FileTaskSendProgressInfo> &info);


    FileTransferSendTask::FileTransferSendTask(DWORD dSrcTermId, DWORD dRemoteId, DWORD dTaskId, std::string file_name,
                                               std::string file_absolute_name) :
            FileTransferTask(dSrcTermId, dRemoteId, dTaskId, file_name, file_absolute_name) {
        //开始依据常量值初始化非常量值
        //一旦有任务创建，将全局内核对象变为受信状态
        //SetEvent(g_bigdata_transfer_event);
        pthread_mutex_init(&_transferStateMutex, nullptr);
        m_p_file.open(file_absolute_name.c_str(), std::ifstream::binary);
        if (!m_p_file.is_open()) {    //文件打开失败
            printf("[BigDataTransferDebug]::because of OpenFailed, task failed\n");
            m_file_state = FILE_SEND_TASK_FAIL;

            printf("File %s open failed!\n", m_file_absolute_name.c_str());
            return;
        }

        _bigdatatransfer_infohub_instance->table_set("bigdatatransfer",
                                                     "fid_sendfilename_map",
                                                     dTaskId/TASK_ID_INCREMENT, file_name);

        //snd_fid_progress_sttable.begin(dTaskId/TASK_ID_INCREMENT, m_file_length);
        //printf("TaskID:%d File %s begin to progress-------->!\n",dTaskId / TASK_ID_INCREMENT,  m_file_absolute_name.c_str());

        m_file_state = FILE_SEND_TASK_ING;
        m_file_length_has_transfer = 0;
        m_file_transfer_speed = 0;
        //m_file_send_length = 0;
        m_timestamp_end = 0;
        m_b_is_file_read_all = false;
    }

    void FileTransferSendTask::DoRecvFileRecvFailed(std::shared_ptr<FileRecvFailed> pRecvMessage) {
        MutexLockGuard gLock(_transferStateMutex);
        //printf("[BigDataTransferDebug]::because of DoRecvFileRecvFailed, task failed\n");
        m_file_state = FILE_SEND_TASK_FAIL;
        //调用回调函数，通知上层模块文件发送任务已经失败
        m_timestamp_end = GetTickCount();
        const DWORD file_time_cost_second = (m_timestamp_end - m_timestamp_start) ;
        shared_ptr<FileTaskSendStatusInfo> info = std::make_shared<FileTaskSendStatusInfo>
                (m_task_id, m_dest_term_id, m_file_name, m_file_absolute_name, m_file_state,
                 m_speed_kbps, file_time_cost_second, m_file_length);
        CallAll_RegisterFileSendTaskStatusCallBack(info);
    }

    void FileTransferSendTask::DoRecvFileSendFailed() {
        MutexLockGuard gLock(_transferStateMutex);
        //printf("[BigDataTransferDebug]::because of DoRecvFileSendFailed, task failed\n");
        m_file_state = FILE_SEND_TASK_FAIL;
        //调用回调函数，通知上层模块文件发送任务已经失败
        m_timestamp_end = GetTickCount();
        const DWORD file_time_cost_second = (m_timestamp_end - m_timestamp_start) ;
        shared_ptr<FileTaskSendStatusInfo> info = std::make_shared<FileTaskSendStatusInfo>
                (m_task_id, m_dest_term_id, m_file_name, m_file_absolute_name, m_file_state,
                 m_speed_kbps, file_time_cost_second, m_file_length);
        CallAll_RegisterFileSendTaskStatusCallBack(info);
    }

    void FileTransferSendTask::DoRecvFileRecvSuccess(const std::shared_ptr<FileRecvSuccess> &pRecvMessage) {
        //函数中更新了两次end时间戳
        MutexLockGuard gLock(_transferStateMutex);
        if (FILE_SEND_TASK_ING == m_file_state) {
//        if (FILE_SEND_TASK_SUCCESS == m_file_state) {
            //调用回调函数，通知上层模块文件发送任务已经成功
            m_timestamp_end = GetTickCount();
            //更新状态
            //mark::
            m_file_state = FILE_SEND_TASK_SUCCESS;
            const DWORD file_time_cost_second = (m_timestamp_end - m_timestamp_start) ;
            shared_ptr<FileTaskSendStatusInfo> info = std::make_shared<FileTaskSendStatusInfo>
                    (m_task_id, m_dest_term_id, m_file_name, m_file_absolute_name, m_file_state, m_speed_kbps,
                     file_time_cost_second, m_file_length);
            CallAll_RegisterFileSendTaskStatusCallBack(info);
            //printf("\n[BigDataTransferDebug]::File Recv Success!\n\n");
            //更新任务结束时间戳
            m_timestamp_end = GetTickCount();
            //关闭文件
            //MARK:20230402--->SegmentFault
            if (m_p_file.is_open()) {
                //m_p_file.close();
            }
        }
    }

    bool FileTransferSendTask::OnRecvData(shared_ptr<BigDataTransfer::AbstractData> pRecvData) {
        /**
		 * 文件发送任务的数据接收方法目前不需要做什么
		 */
        return false;
    }

    bool FileTransferSendTask::OnRecvMessage(shared_ptr<BigDataTransfer::AbstractMessage> pRecvMessage) {
        //DWORD d_message_flag = pRecvMessage->GetMessageFlag();
        if (FileRecvFailed::IsInstanceOf(pRecvMessage)) {
            shared_ptr<FileRecvFailed> pMessage = std::dynamic_pointer_cast<FileRecvFailed>(pRecvMessage);
            DoRecvFileRecvFailed(pMessage);
            return true;
        }

        if (FileRecvSuccess::IsInstanceOf(pRecvMessage)) {
            shared_ptr<FileRecvSuccess> pMessage = std::dynamic_pointer_cast<FileRecvSuccess>(pRecvMessage);
            DoRecvFileRecvSuccess(pMessage);
            return true;
        }
        //return true;
        return false;
    }

    void FileTransferSendTask::SendData() {

        if (FILE_SEND_TASK_FAIL == m_file_state || FILE_SEND_TASK_SUCCESS == m_file_state) {
            //printf("[BigDataTransferDebug]::SendData Finish, Task State : %d\n", m_file_state);
            return;    //表示此任务已经处理完毕了
        }

        const static DWORD d_packet_number_send_once = 5000;//计算一次循环中最多需要发送的包数目
        DWORD d_send_number = 0;
        bool bRet = false;
        //std::shared_ptr<FileSendTransferByteData> p_temp_transfer_data = nullptr;
        MutexLock();
        while (d_send_number < d_packet_number_send_once && 0 != m_buffer_queue.size()) {
            //当发包数目未达到限制且队列未空时
            /*MutexLock();*/
            std::shared_ptr<FileSendTransferByteData> &p_temp_transfer_data = m_buffer_queue.front();
            /*MutexUnLock();*/
            if (Is_RBUDP) {
                bRet = SendDataBytesToTermByRBUDP(m_dest_term_id, p_temp_transfer_data->GetBuffer(),
                                                  p_temp_transfer_data->GetLength());
                /*infohub*/
                bdt_tx_rate_stat.begin();
                bdt_tx_rate_stat.pass(p_temp_transfer_data->GetLength());

                tx_fid_rate_sttable.begin(m_task_id % TASK_ID_INCREMENT);
                tx_fid_rate_sttable.pass(m_task_id % TASK_ID_INCREMENT, p_temp_transfer_data->GetLength());

                snd_fid_progress_sttable.begin(m_task_id%TASK_ID_INCREMENT, m_file_length);
                //printf("TaskID:%d File %s begin to progress-------->!\n",dTaskId / TASK_ID_INCREMENT,  m_file_absolute_name.c_str());
                snd_fid_progress_sttable.pass(m_task_id % TASK_ID_INCREMENT, p_temp_transfer_data->GetLength());

            } else {
                bRet = SendDataBytesToTermByMRUDP(m_dest_term_id, p_temp_transfer_data->GetBuffer(),
                                                  p_temp_transfer_data->GetLength());
                                /*infohub*/
                bdt_tx_rate_stat.begin();
                bdt_tx_rate_stat.pass(p_temp_transfer_data->GetLength());
                tx_fid_rate_sttable.begin(m_task_id % TASK_ID_INCREMENT);
                tx_fid_rate_sttable.pass(m_task_id % TASK_ID_INCREMENT, p_temp_transfer_data->GetLength());
                snd_fid_progress_sttable.begin(m_task_id%TASK_ID_INCREMENT, m_file_length);
                snd_fid_progress_sttable.pass(m_task_id % TASK_ID_INCREMENT, p_temp_transfer_data->GetLength());
            }
            //bRet = SendDataBytesToTermByMRUDP(m_dest_term_id, p_temp_transfer_data->GetBuffer(), p_temp_transfer_data->GetLength());
            //printf("[BDT:AutoTestDebug]::向%d发送数据包，长度是:%d，数据长度为：%d,发送结果:%d\n", m_dest_term_id,
                   //p_temp_transfer_data->GetLength(), p_temp_transfer_data->GetRealDataLength(), bRet);
            if (bRet) {
//                printf("sendTask：sendData.2发送成功\n");
                m_last_transfer_success_time = GetTickCount();    //一定要更新上次传输成功的时间
                m_file_length_has_transfer += p_temp_transfer_data->GetRealDataLength();
                m_packet_transfered_length_count += p_temp_transfer_data->GetRealDataLength();
                //将成功发送的数据从队列中移除
                /*MutexLock();*/
                m_buffer_queue.pop();
                /*MutexUnLock();*/
                //**************************************************CALLBACK********************************
                if (m_b_is_file_read_all && (m_file_length_has_transfer >= m_file_length)) {
                    break;
                } else {
                    //将文件发送任务反馈给相关的需要获取文件发送任务进度信息的模块，可能不止一个
                    static DWORD d_last_progress = 0;    //表示上一次文件发送任务进度
                    DWORD d_current_progress = 100.0 * ((m_file_length_has_transfer * 1.0) / (m_file_length * 1.0));
                    if (1 <= (d_current_progress - d_last_progress)) {

                        //houlc debug
                        printf("-----------------------\n");
                        printf("current progress = %lf\n", d_current_progress);
                        printf("has transfer: %d\n", m_file_length_has_transfer);
                        printf("file length : %d\n", m_file_length);
                        printf("-----------------------\n");

                        ComputeTransferSpeedKbps();    //更新当下传输速率，速率已经计算过了，故略去
                        shared_ptr<FileTaskSendProgressInfo> info = std::make_shared<FileTaskSendProgressInfo>(
                                m_task_id, m_dest_term_id, m_file_name, m_speed_kbps,
                                d_current_progress    //TODO 1000000需要改成文件发送速率
                        );


                        _bigdatatransfer_infohub_instance->table_set("bigdatatransfer",
                           "tx_fid_speedkBps_sttable", m_task_id, m_speed_kbps);
                        

//                        printf("FileSendTask:sendData.1\n");
                        CallAll_RegisterFileSendTaskProgressCallBack(info);
                        d_last_progress = d_current_progress;
                    }
                }
                //******************************************************************************************
            } else {
                const DWORD now_time = GetTickCount();
                DWORD delay_time = (now_time - m_last_transfer_success_time) / 1000;
                if (delay_time > TRANSFER_SUCCESS_TIME_MAX) {
                    //printf("[BigDataTransferDebug]::File Transfer Failed, because it has used too much time!\n");
                    //判定文件发送任务失效
                    DoRecvFileSendFailed();
                    ComputeTransferSpeedKbps();
                    return;
                }
            }
            d_send_number++;
            /*if (!(d_send_number % 50)) {
                usleep(1000);
            }*/
        }
        MutexUnLock();
        ComputeTransferSpeedKbps();
    }

    /**
    * 这个方法用于定时地构造FileTransferData并调用MRUDP模块的方法发送之
    */
    bool FileTransferSendTask::DoTask() {
        MutexLockGuard gLock(_transferStateMutex);
        if (FILE_SEND_TASK_FAIL == m_file_state || FILE_SEND_TASK_SUCCESS == m_file_state ||
            FILE_SEND_TASK_CANCEL == m_file_state) {
            //printf("TaskID is %d, End With State : %d\n", m_task_id, m_file_state);
            return true;    //返回true表示此任务已经处理完毕了
        }
        if (MAX_BUFFER_NUM < m_buffer_queue.size()) {
            SendData();
            return false;
        }
        MutexLock();
        //如果文件还没有被读取完毕
        if (!m_b_is_file_read_all && FILE_SEND_TASK_ING == m_file_state) {
//            printf("sendTask:Dotask.1正在分包\n");

            //读取字节数组的长度
            const DWORD d_read_length = SINGLE_BUFFER_SIZE * SINGLE_READ_PACKET_NUMBER;
            shared_ptr<BYTE> pBuffer(new BYTE[d_read_length], releaseArrays<BYTE>);

            int32_t dRealLength = m_p_file.read((char *) (pBuffer.get()), d_read_length).gcount();//必须是有符号数
            //说明文件读取完毕或者文件读写出错
            if (m_p_file.eof()) {
                //printf("[BigDataTransferDebug]::文件读取完毕！\n");
                m_b_is_file_read_all = true;
            }
            if (m_p_file.bad()) {
                m_file_state = FILE_SEND_TASK_FAIL;
                //TODO DELETE

                printf("[BigDataTransferTest]::File Send Failed!!!\n");
                m_timestamp_end = GetTickCount();
                //调用回调函数，通知上层模块文件发送任务已经失败
                shared_ptr<FileTaskSendStatusInfo> info = std::make_shared<FileTaskSendStatusInfo>
                        (m_task_id, m_dest_term_id, m_file_name, m_file_absolute_name, m_file_state, m_speed_kbps,
                         (m_timestamp_end - m_timestamp_start) /*毫秒为单位*/,
                         m_file_length);
                printf("sendTask:Dotask.2分包失败\n");
                CallAll_RegisterFileSendTaskStatusCallBack(info);
            }

            if (FILE_SEND_TASK_ING != m_file_state) {
                MutexUnLock();
                return true;
            }

            BYTE *pTemp = pBuffer.get();
            while (dRealLength >= SINGLE_BUFFER_SIZE) {
                shared_ptr<BYTE> pSendBuffer(new BYTE[SINGLE_BUFFER_SIZE], releaseArrays<BYTE>);
                memcpy(pSendBuffer.get(), pTemp, SINGLE_BUFFER_SIZE);
                pTemp += SINGLE_BUFFER_SIZE;
                m_file_packet_has_transfered = m_file_packet_has_transfered + 1;
                dRealLength -= SINGLE_BUFFER_SIZE;
                //构造FileTransferData的实例
                FileTransferData transfer_data(
                        m_task_id, m_file_name,
                        m_file_length, m_packet_number, m_file_packet_has_transfered, m_file_length_has_transfer,
                        SINGLE_BUFFER_SIZE, pSendBuffer
                );
                //构造要发送的实例
                DWORD d_send_length = 0;
                std::shared_ptr<BYTE> p_buffer_need_send = transfer_data.Encode(d_send_length);

                //将其放入数据发送缓存队列中
                std::shared_ptr<FileSendTransferByteData> p_send_data =
                        std::make_shared<FileSendTransferByteData>(d_send_length,
                                                                   p_buffer_need_send/*, FileTransferTask::SINGLE_BUFFER_SIZE*/);
                m_buffer_queue.push(p_send_data);
            }
            if (0 < dRealLength) {
                std::shared_ptr<BYTE> pSendBuffer(new BYTE[dRealLength], releaseArrays<BYTE>);
                memcpy(pSendBuffer.get(), pTemp, dRealLength);
                m_file_packet_has_transfered = m_file_packet_has_transfered + 1;
                //构造FileTransferData的实例
                FileTransferData transfer_data(
                        m_task_id, m_file_name,
                        m_file_length, m_packet_number, m_file_packet_has_transfered, m_file_length_has_transfer,
                        dRealLength, pSendBuffer
                );
                dRealLength = 0;
                //构造要发送的实例
                DWORD d_send_length = 0;
                std::shared_ptr<BYTE> p_buffer_need_send = transfer_data.Encode(d_send_length);

                //将其放入数据发送缓存队列中
                std::shared_ptr<FileSendTransferByteData> p_send_data =
                        std::make_shared<FileSendTransferByteData>(d_send_length, p_buffer_need_send, dRealLength);
                m_buffer_queue.push(p_send_data);
            }
        }
        MutexUnLock();
//        printf("sendTask:Dotask.3开始发送\n");
        SendData();

        return false;
    }

    bool FileTransferSendTask::SetTransferCancel() {
        MutexLockGuard gLock(_transferStateMutex);
        m_file_state = FILE_SEND_TASK_CANCEL;
        //判定文件发送任务取消
        //调用回调函数，通知上层模块文件发送任务已经失败
        m_timestamp_end = GetTickCount();
        const DWORD file_time_cost_second = (m_timestamp_end - m_timestamp_start) ;
        shared_ptr<FileTaskSendStatusInfo> info = std::make_shared<FileTaskSendStatusInfo>
                (m_task_id, m_dest_term_id, m_file_name, m_file_absolute_name, m_file_state,
                 m_speed_kbps, file_time_cost_second, m_file_length);
        CallAll_RegisterFileSendTaskStatusCallBack(info);

        return true;
    }

    FileTransferSendTask::FileTransferSendTask(DWORD dSrcTermId, DWORD dRemoteId, DWORD dTaskId, string file_name,
                                               string file_absolute_name, bool is_RBUDP):
                                               FileTransferTask(dSrcTermId, dRemoteId, dTaskId, file_name, file_absolute_name,is_RBUDP) {
        //开始依据常量值初始化非常量值
        //一旦有任务创建，将全局内核对象变为受信状态
        //SetEvent(g_bigdata_transfer_event);
        pthread_mutex_init(&_transferStateMutex, nullptr);
        m_p_file.open(file_absolute_name.c_str(), std::ifstream::binary);
        if (!m_p_file.is_open()) {    //文件打开失败
            printf("[BigDataTransferDebug]::because of OpenFailed, task failed\n");
            m_file_state = FILE_SEND_TASK_FAIL;
            return;
        }

        m_file_state = FILE_SEND_TASK_ING;
        m_file_length_has_transfer = 0;
        m_file_transfer_speed = 0;
        //m_file_send_length = 0;
        m_timestamp_end = 0;
        m_b_is_file_read_all = false;
        Is_RBUDP=is_RBUDP;
    }
}
