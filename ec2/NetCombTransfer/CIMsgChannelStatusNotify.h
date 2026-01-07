//
// Created by 王炳棋 on 2022/11/15.
//

#ifndef NETCOMBTRANSFER_CIMSGCHANNELSTATUSNOTIFY_H
#define NETCOMBTRANSFER_CIMSGCHANNELSTATUSNOTIFY_H

#include "NetCombTransferBase.h"

class CIMsgChannelStatusNotify
        : public CIMsg {
public:
    DWORD m_dwCID;
private:
    const static int CIMsgChannelStatusNotify_Length = 9;
    const static int CIMsgChannelStatusNotify_Least_Meaning_Length = 9;
public:
    CIMsgChannelStatusNotify() : CIMsg() {
        m_dwCID = 0;
    }

    virtual ~CIMsgChannelStatusNotify() {

    }

public:
    DWORD GetCommandID() const override {
        return eCommandChannelStatusNotify;
    }

    //将此数据包的实例深度复制为另一同类数据包
    std::shared_ptr<CIMsg> CopyFrom() const override;

    //序列化
    std::shared_ptr<BYTE> Encode(DWORD &nLength) override;

    //反序列化
    bool Decode(std::shared_ptr<BYTE> pBuffer, DWORD nLength) override;

    //取得数据报的规定长度
    DWORD GetLength() const override;
};


#endif //NETCOMBTRANSFER_CIMSGCHANNELSTATUSNOTIFY_H
