//
// Created by 王炳棋 on 2022/11/20.
//

#include "NetCombTransferCmdThread.h"
#include "CNetTerminalManager.h"

void PlayThreadCleanUp(void *arg) {
//    printf("NetCombTransfer线程的清理函数已调用，线程终止\n");
}

void *PlayThreadProc(void *argNeeded) {

    pthread_cleanup_push(PlayThreadCleanUp, nullptr);
        auto *pThis = (NetCombTransferCmdThread *) argNeeded;
        if (pThis) {
            try {
                pThis->run();
                /*pthread_testcancel();*/
            } catch (...) {
                /*pthread_testcancel();*/
                usleep(1000 * 1000);
                pThis->run();
            }
        }
        /*while (1) {
            pthread_testcancel();
            if (pThis->m_bKillThread) {
                break;
            }
            if (pThis->m_pManager) {
                auto *pManager = (CNetTerminalManager *) pThis->m_pManager;

                pManager->Run();
            }
        }*/
    pthread_cleanup_pop(0);
    return nullptr;
}


NetCombTransferCmdThread::NetCombTransferCmdThread(void *pObject) {
    m_pManager = pObject;
    m_bKillThread = false;
    m_hPlayThread = 0;
}

NetCombTransferCmdThread::~NetCombTransferCmdThread() {}

bool NetCombTransferCmdThread::startup() {
    m_bKillThread = false;
    if (0 != m_hPlayThread) {
        return true;
    }
    int Error = pthread_create(&m_hPlayThread, nullptr, PlayThreadProc, this);
    if (Error != 0) {
        return false;
    } else {
        return true;
    }
}

//线程的退出应该再认真完善一下，现在是随便写的版本
bool NetCombTransferCmdThread::cancel() {
    m_bKillThread = true;
    if (0 != m_hPlayThread) {
        /*pthread_cancel(m_hPlayThread);*/
        pthread_join(m_hPlayThread, nullptr);
//        printf("[NetCombTransfer]::工作线程终止\n");
        /*sleep(5);*/
        m_hPlayThread = 0;
    }
    return true;
}

void NetCombTransferCmdThread::run() {
    for (;;) {
        try {
            //MARK:不加延时嘛?
            if (NetCombTransferCmdThread::check(false)) {
                break;
            }
            if (m_pManager) {
                auto *pManager = (CNetTerminalManager *) m_pManager;

                pManager->Run();
            }

        } catch (...) {
            if (m_pManager) {
                auto *pManager = (CNetTerminalManager *) m_pManager;
                pManager->Run();
            }
        }
    }
}