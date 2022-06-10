#pragma once

#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable
#include <stdint.h>

class SyncWaitMsg
{
public:
    SyncWaitMsg()
        :_waitingMsg(INVALID_WAIT_MSG)
    {
    }
    ~SyncWaitMsg()
    {
    }

    bool wait(uint32_t msg, uint32_t waitTime = DEFAULT_WAIT_TIME)
    {
        if (INVALID_WAIT_MSG == msg)
            return false;

        _waitingMsg = msg;

        std::unique_lock<std::mutex> lck(_mtx);
        while (!_ready)
        {
            if (_cv.wait_for(lck, std::chrono::seconds(waitTime)) == std::cv_status::timeout)
            {
                _ready = false;
                return false;
            }
        }
        _ready = false;
        return true;
    }

    void receive(uint32_t msg)
    {
        if (msg == _waitingMsg)
        {
            std::unique_lock<std::mutex> lck(_mtx);
            _ready = true;
            _cv.notify_all();
        }
    }

    void reset()
    {
        _waitingMsg = INVALID_WAIT_MSG;
    }
private:
    uint32_t _waitingMsg;

    std::mutex _mtx;
    std::condition_variable _cv;
    bool _ready = false;

    enum { INVALID_WAIT_MSG = 0 };
    enum { DEFAULT_WAIT_TIME  = 99999999 };
};
