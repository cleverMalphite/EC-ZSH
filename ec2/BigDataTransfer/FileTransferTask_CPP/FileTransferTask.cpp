//
// Created by 王炳棋 on 2023/1/12.
//

#include "../MRUDP/MRUDPApi.h"
#include "../RBUDP/RBUDPApi.h"
#include "TempSDKInterfaceParams.h"
#include "FileTransferTask_H/FileTransferTask.h"
#include "DataAndMessage_H/FileSendTaskCancel.h"
#include "DataAndMessage_H/FileRecvSuccess.h"
#include "DataAndMessage_H/FileRecvFailed.h"


namespace BigDataTransfer {
    extern void CallAll_RegisterFileRecvTaskStatusCallBack(const shared_ptr<FileTaskRecvStatusInfo>& info);

    FileTransferTask::FileTransferTask(DWORD dSrcTermId, DWORD dDestTermId, std::string FileAbsoluteName,
                                       shared_ptr<BigDataTransfer::FileTransferData> pData) :
            m_src_term_id(dSrcTermId),
            m_dest_term_id(dDestTermId),
            m_task_id(pData->GetFileTransferTaskId()),
            m_file_name(pData->GetFileName()),
            m_file_absolute_name(FileAbsoluteName),
            m_file_length(pData->GetFileLengthAll()),
            m_timestamp_start(GetTickCount()),
            m_packet_number(pData->GetFilePacketNumber()),
            m_file_packet_has_transfered(0), MIN_COMPUTE_LENGTH_FOR_SPEED(GetMinComputeLengthForSpeed(m_file_length)) {
        TaskRecord.TaskStartTime = GetTickCount();
        pthread_mutex_init(&Mutex_, nullptr);
    }

    bool FileTransferTask::SendFileRecvSuccess() {

        shared_ptr<FileRecvSuccess> p_recv_success_message = std::make_shared<FileRecvSuccess>(m_task_id);

        DWORD d_send_message_bytes_length = 0;

        shared_ptr<BYTE> p_send_message_bytes = p_recv_success_message->Encode(d_send_message_bytes_length);

        //**********************调用回调函数***************************
        m_timestamp_end = GetTickCount();
        ComputeTransferSpeedKbps(true);
        shared_ptr<FileTaskRecvStatusInfo> info = std::make_shared<FileTaskRecvStatusInfo>
                (m_task_id, m_src_term_id, m_file_name, m_file_absolute_name, m_file_state,
                 m_speed_kbps/*接收速率另外确定*/, (m_timestamp_end - m_timestamp_start) , m_file_length);
        CallAll_RegisterFileRecvTaskStatusCallBack(info);
        //***********************************************************
        auto start_time = GetTickCount();
        if(Is_RBUDP){
            while (!SendDataBytesToTermByRBUDP(m_src_term_id, p_send_message_bytes, d_send_message_bytes_length)) {
                auto delay_time = (GetTickCount() - start_time) / 1000;
                if (delay_time > TRANSFER_SUCCESS_TIME_MAX) {
                    return false;
                } else {
                    usleep(1000);
                }
            }
        }
        else {
            while (!SendDataBytesToTermByMRUDP(m_src_term_id, p_send_message_bytes, d_send_message_bytes_length)) {
                auto delay_time = (GetTickCount() - start_time) / 1000;
                if (delay_time > TRANSFER_SUCCESS_TIME_MAX) {
                    return false;
                } else {
                    usleep(1000);
                }
            }
        }
        PrintMRUDPInfo();
        return true;
    }

    DWORD FileTransferTask::ComputeTransferSpeedKbps(bool force) {
        const DWORD now = GetTickCount();
        const DWORD elapsed = now - m_packet_transfered_start_time;

        if (!force && elapsed < SPEED_WINDOW_MS) {
            return m_speed_kbps;
        }

        if (elapsed == 0 || m_packet_transfered_length_count == 0) {
            if (force) {
                m_packet_transfered_start_time = now;
                m_packet_transfered_end_time = now;
            }
            return m_speed_kbps;
        }

        m_packet_transfered_end_time = now;    //一个统计周期结束
        m_speed_kbps = static_cast<float>(m_packet_transfered_length_count) /
                       static_cast<float>(elapsed);    // bytes/ms，沿用现有UI口径
        m_packet_transfered_length_count = 0;
        m_packet_transfered_start_time = now;
        return m_speed_kbps;
    }

    DWORD FileTransferTask::GetFilePacketNumber(DWORD64 d_file_length) {
        DWORD d_packet_number = d_file_length / SINGLE_BUFFER_SIZE;
        if (0 != (d_file_length % SINGLE_BUFFER_SIZE)) {
            //如果文件大小（字节）不是转换后每个包大小的整数倍，那么将此文件转换后应有的包数目加1
            d_packet_number++;
        }

        return d_packet_number;
    }

    bool FileTransferTask::SendFileRecvFailed() {
        //向对端发送文件接收任务失败指令
        shared_ptr<FileRecvFailed> p_recv_failed_message = std::make_shared<FileRecvFailed>(m_task_id);
        DWORD d_send_message_bytes_length = 0;
        shared_ptr<BYTE> p_send_message_bytes = p_recv_failed_message->Encode(d_send_message_bytes_length);

        //**********************调用回调函数*************************
        m_timestamp_end = GetTickCount();
        ComputeTransferSpeedKbps(true);
        shared_ptr<FileTaskRecvStatusInfo> info = std::make_shared<FileTaskRecvStatusInfo>
                (m_task_id, m_src_term_id, m_file_name, m_file_absolute_name, m_file_state, m_speed_kbps/*接收速率另外确定*/,
                 (m_timestamp_end - m_timestamp_start) ,
                 m_file_length);
        CallAll_RegisterFileRecvTaskStatusCallBack(info);
        //***********************************************************
        auto start_time = GetTickCount();
        if (Is_RBUDP) {
            while (!SendDataBytesToTermByRBUDP(m_src_term_id, p_send_message_bytes, d_send_message_bytes_length)) {
                auto delay_time = (GetTickCount() - start_time) / 1000;
                if (delay_time > TRANSFER_SUCCESS_TIME_MAX) {
                    return false;
                } else {
                    usleep(1000);
                }
            }
        }
        else{
            while (!SendDataBytesToTermByMRUDP(m_src_term_id, p_send_message_bytes, d_send_message_bytes_length)) {
                auto delay_time = (GetTickCount() - start_time) / 1000;
                if (delay_time > TRANSFER_SUCCESS_TIME_MAX) {
                    return false;
                } else {
                    usleep(1000);
                }
            }
        }

        return true;
    }

    bool FileTransferTask::SendSendTaskCancel() {
        printf("FileTransferTask::SendSendTaskCancel.1\n");
        shared_ptr<FileSendTaskCancel> p_file_send_cancel_message = std::make_shared<FileSendTaskCancel>(m_task_id);
        DWORD d_send_message_bytes_length = 0;
        shared_ptr<BYTE> p_send_message_bytes = p_file_send_cancel_message->Encode(d_send_message_bytes_length);
        auto start_time = GetTickCount();
        if(Is_RBUDP){
            while (!SendDataBytesToTermByRBUDP(m_src_term_id, p_send_message_bytes, d_send_message_bytes_length)) {
                auto delay_time = (GetTickCount() - start_time) / 1000;
                if (delay_time > TRANSFER_SUCCESS_TIME_MAX) {
                    return false;
                } else {
                    usleep(1000);
                }
            }
        }
        else {
            while (!SendDataBytesToTermByMRUDP(m_src_term_id, p_send_message_bytes, d_send_message_bytes_length)) {
                auto delay_time = (GetTickCount() - start_time) / 1000;
                if (delay_time > TRANSFER_SUCCESS_TIME_MAX) {
                    return false;
                } else {
                    usleep(1000);
                }
            }
        }
        return true;
    }

}
