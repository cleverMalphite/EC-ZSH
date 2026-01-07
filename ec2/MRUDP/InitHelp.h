//
// Created by 王炳棋 on 2022/12/28.
//


#ifndef NETCOMBTRANSFER_INITHELP_H
#define NETCOMBTRANSFER_INITHELP_H

#include "SerializeUtil.h"
#include "End2EndReliableTransmission.h"

namespace MRUDP {

    bool g_bStop;   //为true表示此模块内线程需要优雅关闭，否则表示此模块线程可以优雅进行
    pthread_mutex_t gStopMutex_;    //配合g_bStop使用
    //用于设置线程结束标志
    void SetThreadStop(bool bStop);

    //用于得到线程结束标志的值
    bool GetThreadStop();
}

#endif //NETCOMBTRANSFER_INITHELP_H
