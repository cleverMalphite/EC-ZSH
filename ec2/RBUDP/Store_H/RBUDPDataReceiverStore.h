//
// Created by Kong on 2023/6/9.
//

#ifndef RBUDP_RBUDPDATARECEIVERSTORE_H
#define RBUDP_RBUDPDATARECEIVERSTORE_H


#include "Transfer_H/RBUDPdata.h"

namespace RBUDP
{
    class RBUDPDataReceiverStore :
            public RBUDPdata
    {
        DWORD m_acked_timestamp = 0;	//此数据包第一次被接收时的时间戳

    public:


        DWORD GetSackedTimeStamp()
        {
            return m_acked_timestamp;
        }

    };
}
#endif //RBUDP_RBUDPDATARECEIVERSTORE_H