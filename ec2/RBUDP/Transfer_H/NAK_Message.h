//
// Created by Kong on 2023/6/9.
//



#include "queue"
#include "SerializeUtil_RBUDP.h"

#ifndef RBUDP_NAK_MESSAGE_H
#define RBUDP_NAK_MESSAGE_H

namespace RBUDP {

    class NAK_Message
    {
    private:
        BYTE m_bFlag;								//命令数据标志位，
        //DWORD m_dNAKseq = 0;						//表示当前传输序号
        //DWORD m_dTimeStamp = 0;					//表示生成时间戳
        //DWORD m_dTailPacketNumber = 0;			//表示顺序确认的最后一个包的序号（这个包序号对应的包已经被成功接收），也被称为尾包序号
        //DWORD m_dTailPacketTimeStamp = 0;			//表示尾包的时间戳
        //DWORD m_dNakNumber = 0;
        BYTE m_fFlag;								//NAK生成标志位
        DWORD m_doffset = 0;						//记录偏移量
        DWORD m_dSackBlockNumber = 0;				//SACK区块个数
        std::vector<DWORD> m_sack_blocks;			//SACK区块
        //TODO
    public:
        const static DWORD m_nPacketLengthExceptData = 10;


        NAK_Message(BYTE m_flag, DWORD m_offset, std::vector<DWORD>sack_blocks)
                :m_bFlag(MRUDP_MESSAGE_FIRST_BYTE_FLAG), m_fFlag(m_flag), m_doffset(m_offset), m_dSackBlockNumber(sack_blocks.size()), m_sack_blocks(sack_blocks)
        {
            //SetOffset(m_offset);
            //m_dTailPacketNumber = receiveStore->GetPacketIndex();
            //m_dTailPacketTimeStamp = receiveStore->GetTimeStamp();
            //m_dTailFirstAckedTimeStamp = receiveStore->GetSackedTimeStamp();
            //m_dTailRetransferForTimeout = receiveStore->GetTimeOutReTransferTime();
            //m_dTailRetransferForRequest = receiveStore->GetReuqestReTransferTime();
            //m_dCycleIndex = receiveStore->GetCycleIndex();
        }

        NAK_Message() :
                m_bFlag(MRUDP_MESSAGE_FIRST_BYTE_FLAG),
                //m_dTimeStamp(GetTickCount()), //m_dTailPacketNumber(0), 
                //m_dTailPacketTimeStamp(0),
                //m_dTailCumulativeAckTimeStamp(0), m_dTailFirstAckedTimeStamp(0),
                //m_dTailRetransferForTimeout(0), m_dTailRetransferForRequest(0),
                m_dSackBlockNumber(0)
        {
        };
        ~NAK_Message(void)
        {
        };

    public:
        std::shared_ptr<BYTE> Encode(DWORD& nLength);	//将数据包序列化，并将字节序列化后字节序列的有效长度通过参数传递给调用方
        bool Decode(const std::shared_ptr<BYTE> packet_bytes, DWORD nLength);	//将字节序列反序列化
        DWORD GetOffset() const { return m_doffset; }
        void SetOffset(DWORD dOffset) { m_doffset = dOffset; }
        std::vector<DWORD> GetSackBlocks() { return m_sack_blocks; }
        BYTE GetFlag() const { return m_fFlag; }
        bool segmentation();
    };
}
#endif //RBUDP_NAK_MESSAGE_H