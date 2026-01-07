//
// Created by Kong on 2023/6/9.
//




#include "../mGlobalDef.h"
#ifndef RBUDP_CRBUDPTERM2TERMCONFIG_H
#define RBUDP_CRBUDPTERM2TERMCONFIG_H
namespace RBUDP
{
    /**
     * 这个类抽象出了Role，Channel的二元映射
     */
    class CRBUDPTerm2TermConfig
    {
    public:
        CRBUDPTerm2TermConfig(DWORD d_remote_term_id, DWORD d_self_term_id);
        ~CRBUDPTerm2TermConfig();
        CRBUDPTerm2TermConfig()
        {

        };

        DWORD GetRemoteTermId() { return m_dRemoteTermId; }
        DWORD GetSelfTermId() { return m_dSelfTermId; }
    private:
        DWORD m_dRemoteTermId;	//对端TID
        DWORD m_dSelfTermId;		//本端TID


    };

}

#endif //RBUDP_CRBUDPTERM2TERMCONFIG_H