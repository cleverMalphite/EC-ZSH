//
// Created by 王炳棋 on 2022/11/16.
//

#ifndef NETCOMBTRANSFER_CROUTERINFO_H
#define NETCOMBTRANSFER_CROUTERINFO_H

#include "../mGlobalDef.h"

/*这个类作为路由条目的实体存在*/
class CRouterInfo {
public:
    DWORD m_dwSourceTID;
    DWORD m_dwDestinationTID;
    UINT m_nNop;
    UINT m_nQoSSend;
    UINT m_nQoSRecv;
public:
    CRouterInfo(DWORD dwDestinationTID, DWORD dwSourceTID, UINT nNop, UINT nQoSSend = 0, UINT nQoSRecv = 0) :
            m_dwSourceTID(dwSourceTID), m_dwDestinationTID(dwDestinationTID),
            m_nNop(nNop), m_nQoSSend(nQoSSend), m_nQoSRecv(nQoSRecv) {
    }

    virtual ~CRouterInfo() {}
};

inline bool operator==(const CRouterInfo &lhs, const CRouterInfo &rhs) {
    return (lhs.m_dwDestinationTID == rhs.m_dwDestinationTID) && (lhs.m_dwSourceTID == rhs.m_dwSourceTID);
}

inline bool operator!=(const CRouterInfo &lhs, const CRouterInfo &rhs) {
    return !(operator==(lhs, rhs));
}

inline bool operator<(const CRouterInfo &lhs, const CRouterInfo &rhs) {
    return operator==(lhs, rhs)  && (lhs.m_nNop < rhs.m_nNop);
}

#endif //NETCOMBTRANSFER_CROUTERINFO_H
