//
// Created by Kong on 2023/6/9.
//

#ifndef RBUDP_RBUDPDATASENDERSTORE_H
#define RBUDP_RBUDPDATASENDERSTORE_H


#include "Transfer_H/RBUDPdata.h"

namespace RBUDP
{

    class RBUDPDataSenderStore :
            public RBUDPdata
    {
    private:
        /* data */
        DWORD m_data_bytes_length = 0;    //编码成功后得到的数据字节序列的长度
        DWORD m_retransfer_time = 0;		//用于超时重传
        DWORD m_last_retransfer_time = 0;	//上次进行传输的时间戳
        bool b_abletoTimeout = false;
    public:
        explicit RBUDPDataSenderStore(std::shared_ptr<BYTE> data, const DWORD dLength) :RBUDPdata(data, dLength){
            m_last_retransfer_time= this->m_udTimeStamp;
        }
        explicit RBUDPDataSenderStore() :RBUDPdata(nullptr, 0) {

        }

        bool SetEncodeBytes(DWORD bytes_length) {
            m_data_bytes_length = bytes_length;
            return true;
        }

        DWORD GetLastRetransferTime()
        {
            return m_last_retransfer_time;
        }

        DWORD GetRetransferTime()
        {
            return m_retransfer_time;
        }

        bool GetabletoTimeout() {
            return b_abletoTimeout;
        }

        bool abletoTimeout() {
            b_abletoTimeout = true;
            return b_abletoTimeout;
        }

        bool AddRetransferTime() {
            m_retransfer_time++;
            if (m_retransfer_time >= 10) {
                m_retransfer_time = 2;
                return false;
            }
            return true;
        }

        void SetLastRetransferTime(DWORD last_retransfer_time)
        {
            m_last_retransfer_time = last_retransfer_time;
        }
    };


} // namespace RBUDP
#endif //RBUDP_RBUDPDATASENDERSTORE_H
