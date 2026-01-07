//
// Created by 王炳棋 on 2022/11/20.
//

#ifndef NETCOMBTRANSFER_NETCOMBTRANSFERCMDTHREAD_H
#define NETCOMBTRANSFER_NETCOMBTRANSFERCMDTHREAD_H

#include "../mGlobalDef.h"

class NetCombTransferCmdThread {
public:
    bool m_bKillThread;
    pthread_t m_hPlayThread;

    void *m_pManager;
public:
    NetCombTransferCmdThread(void *pObject);

    ~NetCombTransferCmdThread();

public:

    bool startup();

    bool cancel();

    void run();

    //ms级休眠
    void msleep(int nTime) {
        usleep(nTime * 1000);
    }

    bool check(bool bExit) {
        if (bExit) {
            m_bKillThread = true;
        }
        return m_bKillThread;
    }
};


#endif //NETCOMBTRANSFER_NETCOMBTRANSFERCMDTHREAD_H
