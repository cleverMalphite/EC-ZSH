//
// Created by Kong on 2023/6/9.
//
#include "CRBUDPTerm2TermConfig.h"

namespace RBUDP
{
    CRBUDPTerm2TermConfig::CRBUDPTerm2TermConfig(DWORD d_remote_term_id, DWORD d_self_term_id)
    {
        m_dRemoteTermId = d_remote_term_id;
        m_dSelfTermId = d_self_term_id;
    }

    CRBUDPTerm2TermConfig::~CRBUDPTerm2TermConfig()
    {

    }

}