//
// Created by 王炳棋 on 2022/11/15.
//

#include "CIMsgChannelStatusNotify.h"

//todo:整合到一个.h文件中去
std::shared_ptr<CIMsg> CIMsgChannelStatusNotify::CopyFrom() const {
    std::shared_ptr<CIMsgChannelStatusNotify> pObject = std::make_shared<CIMsgChannelStatusNotify>();
    if (pObject) {
        pObject->m_dwDeviceID = m_dwDeviceID;
        pObject->m_dwCID = m_dwCID;

        return pObject;
    }

    return nullptr;
}

std::shared_ptr<BYTE> CIMsgChannelStatusNotify::Encode(DWORD &nLength) {
    nLength = CIMsgChannelStatusNotify_Length;

    std::shared_ptr<BYTE> pBuffer(new BYTE[nLength], releaseArrays<BYTE>);
    if (pBuffer) {
        BYTE *pTemp = pBuffer.get();
        //命令与数据区别位置1，表示这是命令
        pTemp[0] = 1;

        pTemp += 1;
        DWORD dwData = GetCommandID();
        WriteData(pTemp, dwData);
        pTemp += 4;
        WriteData(pTemp, m_dwCID);

        return pBuffer;
    }

    nLength = 0;
    return nullptr;
}

bool CIMsgChannelStatusNotify::Decode(std::shared_ptr<BYTE> pBuffer, DWORD nLength) {
    if (pBuffer && nLength >= CIMsgChannelStatusNotify_Least_Meaning_Length) {
        BYTE *pTemp = pBuffer.get();
        //略过命令与数据区别位
        pTemp += 1;
        DWORD dwData = ReadData(pTemp);//dwData没有被用到
        pTemp += 4;
        m_dwCID = ReadData(pTemp);

        return true;
    }
    return false;
}

DWORD CIMsgChannelStatusNotify::GetLength() const {
    return CIMsgChannelStatusNotify_Length;
}