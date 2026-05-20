//
// Created by 王炳棋 on 2023/1/11.
//

#ifndef NETCOMBTRANSFER_FILETRANSFERTASK_H
#define NETCOMBTRANSFER_FILETRANSFERTASK_H

#include <utility>

#include "../BigDataTransfer//DataAndMessage_H/FileTransferData.h"
#include "../BigDataTransfer//FileTransferTaskState.h"
#include "../IniHandle/IniHandleApi.h"

namespace BigDataTransfer {
}
namespace {
    DWORD64 GetFileLineLength(std::string file_absolute_name) {
        DWORD64 dResult = 0;
        //开始依据常量值初始化非常量值
        FILE *fp = fopen(file_absolute_name.c_str(), "rb");
        if (nullptr == fp) {
        } else {
            fseek(fp, 0, SEEK_END);
            fgetpos(fp, (fpos_t *) &dResult);
            fseek(fp, 0, SEEK_SET);
            fclose(fp);
        }
        return dResult;
    }

    DWORD64 GetMinComputeLengthForSpeed(DWORD64 file_length) {
        const static DWORD MIN_COMPUTE_LENGTH = 10000000;
        DWORD64 result = file_length / 100;
        if (result > MIN_COMPUTE_LENGTH)
            result = MIN_COMPUTE_LENGTH;
        return result;
    }
}

namespace BigDataTransfer {

    using std::shared_ptr;
    using std::vector;
    using std::queue;
    using std::string;
    struct TransferRecord {
        DWORD64 TaskStartTime = 0;
        DWORD64 TaskEndTime = 0;
        DWORD64 AverageSpeed = 0;
    };

    class FileTransferTask {
    public:
        //
        FileTransferTask(DWORD dSrcTermId, DWORD dDestTermId, DWORD dTaskId, string file_name,
                         const string &file_absolute_name) :
                m_src_term_id(dSrcTermId),
                m_dest_term_id(dDestTermId),
                m_task_id(dTaskId),
                m_file_name(std::move(file_name)),
                m_file_absolute_name(file_absolute_name),
                m_file_length(GetFileLineLength(file_absolute_name)),
                m_timestamp_start(GetTickCount()),
                m_packet_number(GetFilePacketNumber(m_file_length)),
                m_file_packet_has_transfered(0),
                MIN_COMPUTE_LENGTH_FOR_SPEED(GetMinComputeLengthForSpeed(m_file_length)) {
            /*printf("[BigDataTransferDebug]::Create FileTransfer Task with %d DataPackets, %d ---> %d\n",
                   m_packet_number, m_src_term_id, m_dest_term_id);*/
            TaskRecord.TaskStartTime = GetTickCount();
            TaskRecord.TaskEndTime = 0;
            TaskRecord.AverageSpeed = 0;
            pthread_mutex_init(&Mutex_, nullptr);
        }
        FileTransferTask(DWORD dSrcTermId, DWORD dDestTermId, DWORD dTaskId, string file_name,
                         const string &file_absolute_name,bool is_RBUDP) :
                m_src_term_id(dSrcTermId),
                m_dest_term_id(dDestTermId),
                m_task_id(dTaskId),
                m_file_name(std::move(file_name)),
                m_file_absolute_name(file_absolute_name),
                m_file_length(GetFileLineLength(file_absolute_name)),
                m_timestamp_start(GetTickCount()),
                m_packet_number(GetFilePacketNumber(m_file_length)),
                m_file_packet_has_transfered(0),
                MIN_COMPUTE_LENGTH_FOR_SPEED(GetMinComputeLengthForSpeed(m_file_length)),
                Is_RBUDP(is_RBUDP){
            /*printf("[BigDataTransferDebug]::Create FileTransfer Task with %d DataPackets, %d ---> %d\n",
                   m_packet_number, m_src_term_id, m_dest_term_id);*/
            TaskRecord.TaskStartTime = GetTickCount();
            TaskRecord.TaskEndTime = 0;
            TaskRecord.AverageSpeed = 0;
            pthread_mutex_init(&Mutex_, nullptr);
        }

        FileTransferTask(DWORD dSrcTermId, DWORD dDestTermId, string FileAbsoluteName,
                         shared_ptr<FileTransferData> pData);

        virtual ~FileTransferTask() {
            DWORD transfer_time = m_packet_transfered_end_time - m_packet_transfered_start_time;
            float send_speed_bps = m_file_length_has_transfer * 8 / ((transfer_time + 0.5) / 1000);
            UpdateTransferRecord();
            /*printf("\n[TaskId:%u with %d Packet]::Runtime is %lu s, AverageSpeed is %lu Mbps\n", m_task_id,
                   m_packet_number,
                   (TaskRecord.TaskEndTime - TaskRecord.TaskStartTime) / 1000, TaskRecord.AverageSpeed);*/
            pthread_mutex_destroy(&Mutex_);
        }

    public:
        TransferRecord TaskRecord;

        void UpdateTransferRecord() {
            TaskRecord.TaskEndTime = GetTickCount();
            DWORD64 Runtime = (TaskRecord.TaskEndTime - TaskRecord.TaskStartTime) / 1000;
            if (Runtime)
                TaskRecord.AverageSpeed = ((m_packet_number * SINGLE_BUFFER_SIZE * 8) / 100000) / Runtime;
            else
                TaskRecord.AverageSpeed = ((m_packet_number * SINGLE_BUFFER_SIZE * 8) / 100000) / 1;
        }

    public:
        DWORD GetTransferSpeedKbps() const { return m_speed_kbps; }

    public:
        /**
		 * 这些方法都必须是同步的
		 */
        virtual bool OnRecvData(shared_ptr<AbstractData> pRecvData) { return true; };        //此方法仅供文件接收任务类真实地实现

        virtual bool OnRecvMessage(shared_ptr<AbstractMessage> pRecvMessage) { return true; };    //此方法接收文件接收或发送任务的消息

        //使得文件传输任务为失效
        virtual bool SetTransferCancel() = 0;

        /**
         * 返回true表示某一任务已经处理完毕，返回false表示某一任务还未处理完毕
         */
        virtual bool DoTask() { return false; };

    protected:
        const DWORD m_src_term_id;   //源终端终端号
        const DWORD m_dest_term_id;  //目的终端终端号
        const DWORD m_task_id;       //任务ID
        const string m_file_name;    //文件名，如果用于发送端，就是要发送的文件在对端应有的文件名。如果是接收端，就是所接收文件的文件名，与m_file_absolute_name对应。
        const string m_file_absolute_name;    //文件的绝对路径
        const DWORD64 m_file_length;          //文件的长度（字节为单位）
        const DWORD m_timestamp_start;        //开始此任务的时间戳
        const DWORD m_packet_number;          //此文件应该被转换为多少个数据包
        //中间状态一直在变的变量
        DWORD64 m_file_length_has_transfer = 0;    //文件已经被传输的长度，对于文件接收任务是已经接收到的文件长度，否则为已经发送的文件长度
        DWORD m_file_state;                   //文件状态
        DWORD m_file_transfer_speed;          //文件的传输速率（kbps），这一速率是此文件的平均传输速率
        DWORD m_file_packet_has_transfered;   //文件已经被传输的包数目
        //中间状态不会失效的变量
        DWORD m_timestamp_end;                //关闭此任务的时间戳
        DWORD m_last_transfer_success_time = GetTickCount();   //上一次成功传输（发送或者接收）文件内数据的时间
        const DWORD TRANSFER_SUCCESS_TIME_MAX = 8000;          //如果m_last_transfer_success_time比现在的时间大TRANSFER_SUCCESS_TIME_MAX秒往上，判定任务失败
        bool Is_RBUDP = false; // 默认是MRUDP
    protected:
        //计算文件传输速率
        uint64_t m_packet_transfered_length_count = 0;    //一个传输速率统计周期的总传输字节数目
        DWORD m_packet_transfered_start_time = GetTickCount();    //一个传输速率统计周期的开始时间戳
        DWORD m_packet_transfered_end_time = GetTickCount();    //一个传输速率统计周期的结束时间戳
        float m_speed_kbps = 1;        //文件的实时传输带宽
        const uint64_t MIN_COMPUTE_LENGTH_FOR_SPEED;    //计算实时传输带宽的最小统计量，小于这个统计量的话统计周期顺延，一定要声明在m_file_length之后

    protected:

    public:
        const static DWORD SINGLE_BUFFER_SIZE = 1300;   //从文件中一次读取的字节数目
        const static DWORD SINGLE_READ_PACKET_NUMBER = 100; //一次读取文件中所需要读的包数目
        const static DWORD MAX_BUFFER_NUM = 100000;
        const static DWORD SPEED_WINDOW_MS = 1000;      // 统一发送/接收速率统计窗口

    protected:
        pthread_mutex_t Mutex_;

    protected:
        void MutexLock() {
            pthread_mutex_lock(&Mutex_);
        }

        void MutexUnLock() {
            pthread_mutex_unlock(&Mutex_);
        }

        DWORD ComputeTransferSpeedKbps(bool force = false);    //计算文件传输实时带宽，并返回计算后的实时带宽

        DWORD GetFilePacketNumber(DWORD64 d_file_length);

        /**
		 * 发送指令的功能性函数
		 */
        bool SendFileRecvFailed();

        bool SendFileRecvSuccess();

        bool SendSendTaskCancel();
    };

}


#endif //NETCOMBTRANSFER_FILETRANSFERTASK_H
