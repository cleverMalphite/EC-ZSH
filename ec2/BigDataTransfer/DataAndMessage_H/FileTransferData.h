//
// Created by 王炳棋 on 2023/1/11.
//

#ifndef NETCOMBTRANSFER_FILETRANSFERDATA_H
#define NETCOMBTRANSFER_FILETRANSFERDATA_H

#include <utility>

#include "AbstractDataOrMessage.h"

namespace BigDataTransfer {
    class FileTransferData : public AbstractData {
    public:
        FileTransferData(
                const DWORD d_file_transfer_task_id,
                std::string s_file_name,
                const DWORD64 d_file_length_all,
                const DWORD d_file_packet_number,
                const DWORD d_file_packet_has_transfered_number,
                const DWORD64 d_file_already_send_length,
                const DWORD d_data_length,
                std::shared_ptr<BYTE> p_data
        ) : m_file_transfer_task_id(d_file_transfer_task_id),
            m_file_name(std::move(s_file_name)),
            m_file_name_length(m_file_name.size() + 1), /*这里要注意包括最后一个结尾处的字符*/
            m_file_length_all(d_file_length_all),
            m_file_packet_number(d_file_packet_number),
            m_file_already_send_length(d_file_already_send_length),
            m_data_length(d_data_length),
            m_data(std::move(p_data)),
            m_packet_has_transfered_number(d_file_packet_has_transfered_number) {
        }

        FileTransferData() = default;

    private:
        /**
		 * 需要序列化的一堆成员变量
		 * 包头：
		 * 首字节标志（1字节）+文件传输任务的ID（4字节）+文件的总长度（4字节）+
		 * 文件已经发送的长度（4字节）+文件名长度（4字节）+这次传输的数据的长度（4字节）+
		 * 包内容：
		 * 文件名 + 这次data中包含的字节序列
		 */
        DWORD m_file_transfer_task_id{};  //文件传输任务ID
        std::string m_file_name;    //文件名，接收端接收此文件后所生成文件的文件名
        DWORD m_file_name_length{};   //文件名的长度,不包括文件名的最后一个字节，即结尾常用的'\0'
        DWORD64 m_file_length_all{};  //文件的总长度
        DWORD m_file_packet_number{}; //文件被转换为数据包集合后，数据包集合的元素数目
        DWORD64 m_file_already_send_length{}; //上次已经发送到了的文件位置，如上次已经发送了X字节，这次从X+1字节开始发送，则此值为X
        DWORD m_data_length{};    //这次data中包含的字节序列的长度
        std::shared_ptr<BYTE> m_data;   //这次包含的字节序列的内存指针
        DWORD m_packet_has_transfered_number{};   //这是第N个被顺序传输的数据包
    private:
        const static DWORD m_dFlag = 10;
        const static DWORD m_packet_header_length = 37;
    public:
        virtual std::shared_ptr<BYTE> Encode(DWORD &dDataLength);

        virtual bool Decode(std::shared_ptr<BYTE> pBuffer, DWORD dLength);

        /**
         * 如果字节序列对应的是此消息就返回true，否则返回false
         */
        static bool IsEncodeInstanceOf(std::shared_ptr<BYTE> pData, DWORD dDataLength);

    public:
        DWORD GetFileTransferTaskId() const { return m_file_transfer_task_id; }

        std::string GetFileName() const { return m_file_name; }

        DWORD GetFileNameLength() const { return m_file_name_length; }

        DWORD64 GetFileLengthAll() const { return m_file_length_all; }

        DWORD64 GetFileAlreadySendLength() const { return m_file_already_send_length; }

        DWORD GetDataLength() const { return m_data_length; }

        std::shared_ptr<BYTE> GetData() const { return m_data; }

        DWORD GetFilePacketNumber() const { return m_file_packet_number; }

        DWORD GetCurrentTransferLenth() const { return m_file_already_send_length; }

        DWORD GetCurrentPacketNumberHasTransfered() const { return m_packet_has_transfered_number; }
    };
}
#endif //NETCOMBTRANSFER_FILETRANSFERDATA_H
