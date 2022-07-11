#pragma once

#include <chrono>
#include <thread>

#include "app_options.hpp"

class Thread
{
public:
    Thread(AppOptions *opts);
    ~Thread();

    bool IsRunning();
    void Start();
    void Stop();

    void SetInterval(std::chrono::microseconds time);

protected:
    virtual void ThreadFunc() = 0;

    AppOptions                  *mOptions;

private:
    void ThreadLoop();

    bool                        mStop;
    std::thread                 mThread;

    std::chrono::microseconds   mInterval;
};
