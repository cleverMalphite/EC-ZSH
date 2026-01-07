//
// Created by Kong on 2023/6/12.
//

#include "RBUDPTempData.h"
namespace RBUDP
{
    RBUDPTempData::RBUDPTempData(DWORD dTermID, shared_ptr<BYTE> pBuffer, DWORD dDataLength)
    {
        m_dTermId = dTermID;
        m_pBuffer = pBuffer;
        m_dDataLength = dDataLength;
    }

    RBUDPTempData::~RBUDPTempData()
    {
    }

    DWORD RBUDPTempData::GetTermId()
    {
        return m_dTermId;
    }

    std::shared_ptr<BYTE> RBUDPTempData::GetpBuffer()
    {
        return m_pBuffer;
    }

    DWORD RBUDPTempData::GetDataLength()
    {
        return m_dDataLength;
    }
}

