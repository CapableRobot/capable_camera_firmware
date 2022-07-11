#pragma once

#include <deque>
#include <chrono>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
using namespace std::chrono;
using namespace std::chrono_literals;

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include "app_options.hpp"
#include "thread.hpp"

#define NUM_QUEUES  2

class Logger : public Thread
{
public:
    Logger(AppOptions *opts);
    virtual ~Logger();

    void SetFileDuration(seconds &&duration);
    
protected:
    void QueueData(json &data);
    
    static std::string GetDateTimeString(timespec time);

    short           mQueueIndex;

private:
    struct FileData
    {
        std::string     name;
        off_t           size;
        time_t          epoch;
    };

    void OpenLog();
    void CheckLogStatus();
    void GetLogData();
    void RotateLogs();
    void ProcessData(short queueIndex);

    virtual void ThreadFunc() override;

    bool            mLogOpen;

    seconds         mDuration;

    std::string     mFileName;
    std::fstream    mLogFile;

    int             mCurrLogSize;
    int             mTotalLogSize;

    json            mOutput;

    std::deque<FileData>        mLogFileQueue;
    std::queue<json>            mDataQueue[NUM_QUEUES];

    steady_clock::time_point    mLogOpenTime;

};
