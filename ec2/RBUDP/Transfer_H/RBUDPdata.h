//
// Created by Kong on 2023/6/9.
//




#include "SerializeUtil_RBUDP.h"
#include "../epollComm/CIPSOCKET.h"
#ifndef RBUDP_RBUDPDATA_H
#define RBUDP_RBUDPDATA_H
namespace RBUDP
{
    const DWORD DATA_PACKET_LENGTH = 1300;



    /**
     * 这个类属于实时UDP可靠传输的数据包实体
     * 主要的结构如下：
     * 一个字节的标志位 + 四个字节的包序号 + 四个字节的时间戳 + 四个字节的数据部分长度 + 数据部分
     * 需要注意的是，这个类不仅仅用于数据包的编解码，还用于构成循环队列文件的数组部分
     */
    class RBUDPdata
    {
    public:
        RBUDPdata(std::shared_ptr<unsigned char> sData, DWORD uwDataLength) :m_uwDataLength(uwDataLength)
        {
            m_udTimeStamp = 0;
            //m_transfer_reason = 0;
            //对字符数组进行深度复制
            std::shared_ptr<BYTE> pTemp(RBUDP::ReadChar(sData.get(), uwDataLength), releaseArrays<BYTE>);
            //m_transfer_reason = RBUDPTransferReason::TRANSFER_FOR_INIT;
            m_sData = pTemp;
        };

        RBUDPdata()
        {
        };

        virtual ~RBUDPdata(void) {

        };

    protected:
        unsigned char m_ucFlag = MRUDP_DATA_FIRST_BYTE_FLAG;	//表示初始发送
        DWORD m_dLength_all = 0;
        DWORD m_dPacketIndex = 0;	//数据包在循环发送队列里的索引
        DWORD m_udTimeStamp = 0;	//四个字节的生成时间戳
        DWORD m_uwDataLength = 0;	//四个字节的数据部分长度
        std::shared_ptr<BYTE> m_sData = nullptr;			//实际有效数据组成的数组
        BYTE m_bPacketUrgentFlag = 0;

    public:
        static const int m_nPacketLengthExceptData = 18;	//表示该数据包处理有效数据之外的部分，即包头
        //static const int m_nPacketLengthReason = 14;	//表示数据传输原因所在的位置
        static const int m_nPacketUrgentFlag = 1;	//紧急传输标志

        std::shared_ptr<BYTE> Encode(DWORD& nLength) const;	//序列化
        bool Decode(std::shared_ptr<BYTE> packet_bytes, DWORD nLength);	//解析数据
        bool ReleaseData();

        bool AddToQueueSuccess(DWORD dPacketIndex, DWORD dLengthall);
        DWORD GetTimeStamp() const { return m_udTimeStamp; }
        static DWORD GetPacketHeaderLength() { return m_nPacketLengthExceptData; }
        DWORD GetDataLength() const { return m_uwDataLength; }
        DWORD GetPacketIndex() const { return m_dPacketIndex; }
        std::shared_ptr<BYTE> GetData() { return m_sData; }
        void SetDataNull() { m_sData = nullptr; }
        //BYTE GetTransferReason() { return m_transfer_reason; }
        BYTE GetPacketUrgentFlag() { return m_bPacketUrgentFlag; }
    };
}
#endif //RBUDP_RBUDPDATA_H
