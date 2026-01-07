//
// Created by 王炳棋 on 2022/12/21.
//

#include "SystemTimeFunc.h"

unsigned long GetTimeStampNow_us() {
    timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

static unsigned long SystemBeginTs_us = GetTimeStampNow_us();

static unsigned int SystemBeginTs_ms = SystemBeginTs_us / 1000;

unsigned int GetTickCount() {
    return GetTimeStampNow_us() / 1000 - SystemBeginTs_ms;
}
