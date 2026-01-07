//
// Created by 王炳棋 on 2022/11/14.
//

#ifndef NETCOMBTRANSFER_CNETBUFFER_H
#define NETCOMBTRANSFER_CNETBUFFER_H

#include "../mGlobalDef.h"

class CNetBuffer {
public:
    UINT m_uSocket;
    std::shared_ptr<BYTE> m_pBuffer;
    DWORD m_dwLength;
    int64_t m_data_recv_timeStamp;
public:
    CNetBuffer();

    CNetBuffer(UINT uSocket, std::shared_ptr<BYTE> pBuffer, unsigned long dwLength, int64_t data_recv_timeStamp);

    ~CNetBuffer();
};


#endif //NETCOMBTRANSFER_CNETBUFFER_H
