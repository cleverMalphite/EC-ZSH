//
// Created by 王炳棋 on 2022/12/31.
//

#ifndef NETCOMBTRANSFER_LOCKGUARD_H
#define NETCOMBTRANSFER_LOCKGUARD_H

#include <cstdint>
#include <cstring>
#include <cassert>
#include <cassert>
#include <pthread.h>

class MutexLockGuard {
public:
    explicit MutexLockGuard(pthread_mutex_t &mutex_) /*__attribute__((acquire_capability(mutex_)))*/
            : Mutex_(mutex_) {
        pthread_mutex_lock(&Mutex_);
    }

    ~MutexLockGuard() {
        pthread_mutex_unlock(&Mutex_);
    }

private:
    pthread_mutex_t& Mutex_;
};

#endif //NETCOMBTRANSFER_LOCKGUARD_H
