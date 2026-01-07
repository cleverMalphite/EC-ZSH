//
// Created by Kong on 2023/6/12.
//

#ifndef RBUDP_RBUDPTEMPDATA_H
#define RBUDP_RBUDPTEMPDATA_H

#endif //RBUDP_RBUDPTEMPDATA_H
#include "../mGlobalDef.h"

using namespace std;

namespace RBUDP
{
    class RBUDPTempData
    {
    public:
        RBUDPTempData(DWORD dTermID, std::shared_ptr<BYTE> pBuffer, DWORD dDataLength);
        RBUDPTempData() {};
        virtual ~RBUDPTempData();

        //返回此数据要交给哪个TermId
        virtual DWORD GetTermId();

        virtual shared_ptr<BYTE> GetpBuffer();

        virtual DWORD GetDataLength();

    private:
        DWORD m_dTermId;
        shared_ptr<BYTE> m_pBuffer;
        DWORD m_dDataLength = 0;
    };
}