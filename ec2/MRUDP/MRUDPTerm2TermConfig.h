//
// Created by 王炳棋 on 2022/12/20.
//

#include "../mGlobalDef.h"

#ifndef MRUDP_MRUDPTERM2TERMCONFIG_H
#define MRUDP_MRUDPTERM2TERMCONFIG_H

namespace MRUDP {
    class MRUDPTerm2TermConfig {
    public:
        MRUDPTerm2TermConfig() = default;

        MRUDPTerm2TermConfig(DWORD d_remote_term_id, DWORD d_self_term_id) :
                m_dRemoteTermId(d_remote_term_id), m_dSelfTermId(d_self_term_id) {

        }

        ~MRUDPTerm2TermConfig() = default;

    public:
        DWORD GetRemoteTermId() const {
            return m_dRemoteTermId;
        }

        DWORD GetSelfTermId() const {
            return m_dSelfTermId;
        }

    private:
        DWORD m_dRemoteTermId{};  //对端TID
        DWORD m_dSelfTermId{};    //本端TID
    };
}

#endif //MRUDP_MRUDPTERM2TERMCONFIG_H
