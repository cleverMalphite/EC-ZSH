//
// Created by 王炳棋 on 2023/1/11.
//

#ifndef NETCOMBTRANSFER_FILESENDTASKCANCEL_H
#define NETCOMBTRANSFER_FILESENDTASKCANCEL_H

#include "AbstractDataOrMessage.h"

namespace BigDataTransfer {
    /**
	 * 上层模块构成此消息，以取消文件发送任务，此消息现在还没有被使用
	 */
    class FileSendTaskCancel : public AbstractMessage {
    public:
        explicit FileSendTaskCancel(DWORD d_file_transfer_task_id) : m_file_transfer_task_id(d_file_transfer_task_id) {};

        FileSendTaskCancel() = default;

        ~FileSendTaskCancel() = default;

    private:
        DWORD m_file_transfer_task_id;    //文件发送任务的ID

    private:
        /**
         * 需要序列化的元素
         */
        const static DWORD m_flag = 1;    //文件发送任务取消的标志
        /**
         * 不需要序列化的元素
         */
        const static DWORD m_packet_length = 5;    //包长度（字节）
    public:

        std::shared_ptr<BYTE> Encode(DWORD &dDataLength);

        bool Decode(std::shared_ptr<BYTE> pBuffer, DWORD dLength);

        /**
         * 如果字节序列对应的是此消息就返回true，否则返回false
         */
        static bool IsEncodeInstanceOf(std::shared_ptr<BYTE> pData, DWORD dDataLength);    //判断是否是解码后为本类型的字节序列

        DWORD GetMessageFlag() override { return m_flag; }

        static bool IsInstanceOf(std::shared_ptr<AbstractMessage> p_message) { return p_message->GetMessageFlag() == m_flag; }

    };
}
#endif //NETCOMBTRANSFER_FILESENDTASKCANCEL_H
