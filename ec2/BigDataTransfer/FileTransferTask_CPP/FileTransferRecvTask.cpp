//
// Created by 王炳棋 on 2023/1/12.
//

#include "TempSDKInterfaceParams.h"
#include "FileTransferTask_H/FileTransferRecvTask.h"
#include "BigDataTransferManager.h"
#include "time.h"

#include "infoHub/infoHub.h"
#include "infoHub/rateStatistic.h"
#include "infoHub/rateStatisticTable.h"
#include "infoHub/progressStatisticTable.h"

extern std::shared_ptr<ec2::infoHub>\
        _bigdatatransfer_infohub_instance ;

extern ec2::rateStatistic      bdt_rx_rate_stat;
extern ec2::rateStatisticTable rx_fid_rate_sttable;

extern ec2::progressStatisticTable rcv_fid_progress_sttable;

extern const DWORD TASK_ID_INCREMENT ;    //一个终端最多能同时传多少个文件


namespace BigDataTransfer {

    extern std::shared_ptr<BigDataTransferManager> gBigDataTransferManager;

    extern void CallAll_RegisterFileRecvTaskStatusCallBack(const shared_ptr<FileTaskRecvStatusInfo> &info);

    extern void CallAll_RegisterFileRecvTaskProgressCallBack(const shared_ptr<FileTaskRecvProgressInfo> &info);

    FileTransferRecvTask::FileTransferRecvTask(DWORD dSrcTermId, DWORD dDestTermId,
                                               std::string FileAbsoluteName,
                                               shared_ptr<BigDataTransfer::FileTransferData> p_data)
            : FileTransferTask(dSrcTermId, dDestTermId, FileAbsoluteName, p_data) {
        pthread_mutex_init(&MyQueMutex_, nullptr);
        //一旦有任务创建，将全局内核对象变为受信状态
        //SetEvent(g_bigdata_transfer_event);
        //移植：考虑使用cond.
        m_file_length_has_transfer = 0;
        m_file_transfer_speed = 0;
        m_timestamp_end = 0;
        m_file_state = FILE_RECV_TASK_ING;

        //打开文件
        m_f_stream.open(m_file_absolute_name.c_str(), std::ofstream::binary);
        if (m_f_stream.fail()) {    //文件打开失败
            //**********************调用回调函数*************************
            m_timestamp_end = GetTickCount();
            m_file_state = FILE_RECV_TASK_FAILED;
            shared_ptr<FileTaskRecvStatusInfo> info = std::make_shared<FileTaskRecvStatusInfo>
                    (m_task_id, m_src_term_id, m_file_name, m_file_absolute_name, m_file_state, m_speed_kbps,
                     (m_timestamp_end - m_timestamp_start) ,
                     m_file_length);
            CallAll_RegisterFileRecvTaskStatusCallBack(info);
            //***********************************************************
            m_f_stream.close();

            printf("File %s open failed!\n", m_file_absolute_name.c_str());

            return;
        } else {
            m_file_state = FILE_RECV_TASK_ING;
            shared_ptr<FileTaskRecvStatusInfo> info = std::make_shared<FileTaskRecvStatusInfo>
                    (m_task_id, m_src_term_id, m_file_name, m_file_absolute_name, m_file_state, m_speed_kbps,
                     (m_timestamp_end - m_timestamp_start),
                     m_file_length);
            CallAll_RegisterFileRecvTaskStatusCallBack(info);

            _bigdatatransfer_infohub_instance->table_set("bigdatatransfer",                           
                                                     "fid_recvfilename_map", 
                                                     m_task_id % TASK_ID_INCREMENT, m_file_name);
 
            //rcv_fid_progress_sttable.begin(m_task_id / TASK_ID_INCREMENT, m_file_length);

        }
        //MARK:Do not invoke virtual member functions from constructor
        OnRecvData(p_data);
        /*printf("[BigDataTransferDebug]::File(recv) {%s} begin to recv data, with total %d Packets\n", m_file_absolute_name.c_str(),
               m_packet_number);*/
    }

    bool FileTransferRecvTask::OnRecvMessage(shared_ptr<BigDataTransfer::AbstractMessage> pRecvMessage) {
        return true;
    }

    bool FileTransferRecvTask::OnRecvData(shared_ptr<BigDataTransfer::AbstractData> pRecvData) {
        if (FILE_RECV_TASK_FAILED == m_file_state || FILE_RECV_TASK_SUCCESS == m_file_state) {
            return false;
        }
        pthread_mutex_lock(&MyQueMutex_);
        shared_ptr<FileTransferData> p_recv_transfer_data = std::dynamic_pointer_cast<FileTransferData>(pRecvData);
        //TODO DELETE
        DWORD d_data_length = p_recv_transfer_data->GetDataLength();
        m_p_recv_data_queue.push(p_recv_transfer_data);
        pthread_mutex_unlock(&MyQueMutex_);


        return true;
    }

    bool FileTransferRecvTask::SetTransferCancel() {
        m_file_state = FILE_RECV_TASK_CANCEL;
        return true;
    }

    bool FileTransferRecvTask::DoTask() {
        const static DWORD d_single_time_deal_packet_number = 5000;
        bool b_send_message_result = false;    //反应向对端发送信令成功与否
        MutexLock();
        if (FILE_RECV_TASK_ING == m_file_state) {
            shared_ptr<FileTransferData> p_transfer_data = nullptr;
            DWORD d_deal_number = 0;
            DWORD dResult = 0;
            pthread_mutex_lock(&MyQueMutex_);

            if (0 == m_p_recv_data_queue.size()) {
                const DWORD now_time = GetTickCount();
                DWORD delay_time = (now_time - m_last_transfer_success_time) / 1000;
                if (delay_time > TRANSFER_SUCCESS_TIME_MAX) {
                    //判定文件接收任务失效
                    m_file_state = FILE_RECV_TASK_FAILED;
                    //printf("[BigDataTransferDebug]::File(recv) %s recv failed because delay!\n", m_file_absolute_name.c_str());
                    std::cout << "【FileTransferRecvTask::DoTask】File(recv) " << m_file_absolute_name
                              << " recv failed because delay!" << std::endl;
                    b_send_message_result = SendFileRecvFailed();
                    CloseAndFlushFileAndCallBack();
                }
            }

            while (0 != m_p_recv_data_queue.size() && d_deal_number <= d_single_time_deal_packet_number) {

                p_transfer_data = m_p_recv_data_queue.front();
                m_f_stream.write((const char *) (p_transfer_data->GetData().get()), p_transfer_data->GetDataLength());
                //fflush(m_p_file);
                m_file_length_has_transfer += p_transfer_data->GetDataLength();
                m_packet_transfered_length_count += p_transfer_data->GetDataLength();
                ComputeTransferSpeedKbps();

                
                _bigdatatransfer_infohub_instance->table_set("bigdatatransfer",                           
                   "rx_fid_speedkBps_sttable", m_task_id % TASK_ID_INCREMENT, m_speed_kbps);

                _bigdatatransfer_infohub_instance->value_set("bigdatatransfer",                           
                   "rx_current_speedkBps_stat",  m_speed_kbps);

                /*infohub*/
                bdt_rx_rate_stat.begin();
                bdt_rx_rate_stat.pass(p_transfer_data->GetDataLength());

                rx_fid_rate_sttable.begin(m_task_id % TASK_ID_INCREMENT);
                rx_fid_rate_sttable.pass(m_task_id % TASK_ID_INCREMENT, p_transfer_data->GetDataLength());

                rcv_fid_progress_sttable.begin(m_task_id % TASK_ID_INCREMENT, m_file_length);
                rcv_fid_progress_sttable.pass(m_task_id % TASK_ID_INCREMENT, p_transfer_data->GetDataLength());

                if (m_f_stream.fail()) {
                    m_file_state = FILE_RECV_TASK_FAILED;
                    //printf("[BigDataTransferDebug]::File(recv) %s recv failed because file write wrong!\n", m_file_absolute_name.c_str());
                    std::cout << "【FileTransferRecvTask::DoTask】File(recv) " << m_file_absolute_name
                              << " recv failed because file write wrong!" << std::endl;
                    b_send_message_result = SendFileRecvFailed();
                    CloseAndFlushFileAndCallBack();
                    break;
                }
                m_file_packet_has_transfered++;
                //**************************************************CALLBACK********************************
                //TODO 将文件接收任务反馈给相关的需要获取文件接收任务进度信息的模块，可能不止一个
                static DWORD d_last_progress = 0;    //表示上一次文件发送任务进度
                DWORD d_current_progress = 100.0 * ((m_file_packet_has_transfered * 1.0) / (m_packet_number * 1.0));
                if (1 <= (d_current_progress - d_last_progress)) {
                    d_last_progress = d_current_progress;
                    shared_ptr<FileTaskRecvProgressInfo> info = std::make_shared<FileTaskRecvProgressInfo>(
                            m_task_id, m_src_term_id, m_file_name, m_speed_kbps,
                            d_last_progress    //TODO 1000000需要改成文件发送速率
                    );
//                    printf("FileRecvTask:DoTask.1\n");
                    CallAll_RegisterFileRecvTaskProgressCallBack(info);
                }
                //******************************************************************************************
                //未按序接收，文件传输错误
                if (m_file_packet_has_transfered != p_transfer_data->GetCurrentPacketNumberHasTransfered()) {
                    m_file_state = FILE_RECV_TASK_FAILED;
                    /*printf("[BigDataTransferDebug]::File(recv) %s recv failed because transfer index miss! NeedRecv:%d,Id:%d RealRecv:%d,Id:%d\n",
                           m_file_absolute_name.c_str(), m_file_packet_has_transfered, m_task_id,
                           p_transfer_data->GetCurrentPacketNumberHasTransfered(), p_transfer_data->GetFileTransferTaskId());*/
                    std::cout << "【FileTransferRecvTask::DoTask】File(recv) " << m_file_absolute_name
                              << " recv failed because transfer index miss!" << std::endl;
                    b_send_message_result = SendFileRecvFailed();
                    //TODO DELETE
                    CloseAndFlushFileAndCallBack();
                    break;
                }
                //m_p_cache_data_queue.push(p_transfer_data);
                m_p_recv_data_queue.pop();
                d_deal_number++;
                m_last_transfer_success_time = GetTickCount();
            }
            pthread_mutex_unlock(&MyQueMutex_);
            //FlushFileAndClearCache();
            if (m_file_packet_has_transfered == m_packet_number && m_file_state != FILE_RECV_TASK_FAILED) {
                m_file_state = FILE_RECV_TASK_SUCCESS;
                //更新任务结束时间戳
                m_timestamp_end = GetTickCount();
                //计算文件接收速率
                CloseAndFlushFileAndCallBack();
            } else if (m_file_packet_has_transfered > m_packet_number && m_file_state != FILE_RECV_TASK_FAILED) {
                //文件接收任务所接收的数据不应该大于文件的总长度
                m_file_state = FILE_RECV_TASK_FAILED;
                /*printf("[BigDataTransferDebug]::File(recv) %s recv failed because file length wrong, %d / %d!!\n",
                       m_file_absolute_name.c_str(),
                       m_file_packet_has_transfered, m_packet_number);*/
                std::cout << "【FileTransferRecvTask::DoTask】File(recv) " << m_file_absolute_name
                          << " recv failed because file length wrong ..." << std::endl;
                b_send_message_result = SendFileRecvFailed();
                CloseAndFlushFileAndCallBack();
            }
        } else if (FILE_RECV_TASK_SUCCESS == m_file_state) {
            b_send_message_result = SendFileRecvSuccess();
            std::cout << "recv Success" << std::endl;
            time_t now = time(nullptr);
            char* curr_time = ctime(&now);
            std::cout << curr_time <<std::endl;
            CloseAndFlushFileAndCallBack();
        } else {
            /*printf("[BigDataTransferDebug]::File(recv) %s recv failed!\n", m_file_absolute_name.c_str());*/
            b_send_message_result = SendFileRecvFailed();
            std::cout << "【FileTransferRecvTask::DoTask】File(recv) " << m_file_absolute_name
                      << " recv failed" << std::endl;
            CloseAndFlushFileAndCallBack();
        }
        MutexUnLock();
        if (b_send_message_result) {
//            printf("TaskID is %d, End With State : %d\n", m_task_id, m_file_state);
        }
        return b_send_message_result;
    }

    void FileTransferRecvTask::CloseAndFlushFileAndCallBack() {
        if (m_f_stream.is_open()) {
            m_f_stream.flush();
            m_f_stream.close();
        }
        m_p_cache_data_queue = queue<std::shared_ptr<FileTransferData> >();
    }

    void FileTransferRecvTask::FlushFileAndClearCache() {
        if (m_f_stream.is_open()) {
            m_f_stream.flush();
        }
        m_p_cache_data_queue = queue<std::shared_ptr<FileTransferData> >();
    }

}
