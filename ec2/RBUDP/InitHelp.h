//
// Created by Kong on 2023/6/9.
//

#ifndef RBUDP_INITHELP_H
#define RBUDP_INITHELP_H



#include "SerializeUtil_RBUDP.h"
#include "End2EndReliableTransmission.h"

namespace RBUDP {

    bool g_bStop;   //为true表示此模块内线程需要优雅关闭，否则表示此模块线程可以优雅进行
    pthread_mutex_t gStopMutex_;    //配合g_bStop使用

    //用于设置线程结束标志
    void SetThreadStop(bool bStop);

    //用于得到线程结束标志的值
    bool GetThreadStop();
}

#endif //RBUDP_INITHELP_H
