#ifndef HV_TIMER_THREAD_HPP_
#define HV_TIMER_THREAD_HPP_

#include <hv/EventLoopThread.h>

namespace hv {

//继承自时间循环线程，可独立使用
class TimerThread : public EventLoopThread {
public:
    std::atomic<TimerID> nextTimerID;
    TimerThread() : EventLoopThread() {
        nextTimerID = 0;
        start();
    }

    virtual ~TimerThread() {
        stop();
        join();
    }

public:
    // setTimer, setTimeout, killTimer, resetTimer thread-safe
    TimerID setTimer(int timeout_ms, TimerCallback cb, uint32_t repeat = INFINITE) {
        TimerID timerID = ++nextTimerID;
        loop()->setTimerInLoop(timeout_ms, cb, repeat, timerID);
        return timerID;
    }
    // alias javascript setTimeout, setInterval
    TimerID setTimeout(int timeout_ms, TimerCallback cb) {
        return setTimer(timeout_ms, cb, 1);
    }
    TimerID setInterval(int interval_ms, TimerCallback cb) {
        return setTimer(interval_ms, cb, INFINITE);
    }

    void killTimer(TimerID timerID) {
        loop()->killTimer(timerID);
    }

    void resetTimer(TimerID timerID, int timeout_ms = 0) {
        loop()->resetTimer(timerID, timeout_ms);
    }
};

//事件定时器，需与EventLoopThreadPool配合使用
class EventTimer{
public:
    //EventLoopThread 
    std::shared_ptr<EventLoopThreadPool>  _pEventLoopPool;
    std::atomic<TimerID> _nextTimerID;

    EventTimer()  = delete ;
    EventTimer(std::shared_ptr<EventLoopThreadPool> _pEventLoopPool)
    {  
        this->_pEventLoopPool = _pEventLoopPool;
        this->_nextTimerID = 0; 
    }

    virtual ~EventTimer() {
    }

public:
    // setTimer, setTimeout, killTimer, resetTimer thread-safe
    TimerID setTimer(int timeout_ms, TimerCallback cb, uint32_t repeat = INFINITE) {
        TimerID timerID = ++_nextTimerID;
        EventLoopPtr loop = _pEventLoopPool->nextLoop();
        loop->setTimerInLoop(timeout_ms, cb, repeat, timerID);
        return timerID;
    }
    // alias javascript setTimeout, setInterval
    TimerID setTimeout(int timeout_ms, TimerCallback cb) {
        return setTimer(timeout_ms, cb, 1);
    }
    TimerID setInterval(int interval_ms, TimerCallback cb) {
        return setTimer(interval_ms, cb, INFINITE);
    }

    //TODO 这个有问题，可能删除别的线程的定时器
    void killTimer(TimerID timerID) {
        for(int i = 0; i < _pEventLoopPool->threadNum(); i++){
            EventLoopPtr loop = _pEventLoopPool->nextLoop();
            loop->killTimer(timerID);
        }
    }

    //TODO 这个有问题，可能修改别的线程的定时器
    void resetTimer(TimerID timerID, int timeout_ms = 0) {
        for(int i = 0; i < _pEventLoopPool->threadNum(); i++){
            EventLoopPtr loop = _pEventLoopPool->nextLoop();
            loop->resetTimer(timerID, timeout_ms);
        }
    }
};

} // end namespace hv

#endif // HV_TIMER_THREAD_HPP_
