#include "thread.hpp"

#include <chrono>
#include <iostream>
#include <thread>

#include "app_options.hpp"

using namespace std::chrono_literals;

Thread::Thread(AppOptions *opts) : mOptions(opts), mStop(false), 
    mInterval(0us) {};

Thread::~Thread()
{
    Stop();
}

bool Thread::IsRunning()
{
    return (mStop == false);
}

void Thread::Start()
{
    // If the thread isn't joinable there is no thread running
    if (mThread.joinable() == false)
    {
        // Create a new thread
        mThread = std::thread(&Thread::ThreadLoop, this);
    }
}

void Thread::Stop()
{
    // If there is no thread running
    if (IsRunning() == true)
    {
        if (mOptions->verbose == true)
        {
            std::cerr << "Stopping loop thread." << std::endl;
        }

        // Change loop flag and wait for the thread to join
        mStop = true;
        mThread.join();

        if (mOptions->verbose == true)
        {
            std::cerr << "Thread joined." << std::endl;
        }
    }
}

void Thread::SetInterval(std::chrono::microseconds time)
{
    mInterval = time;
}

void Thread::ThreadLoop()
{
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;

    // Loop until the run flag has changed
    while (mStop == false)
    {
        // If there's an interval get the start time
        if (mInterval.count() != 0)
        {
            startTime = std::chrono::steady_clock::now();
        }

        // Call the thread loop function
        ThreadFunc();

        // If there's an interval set, sleep the thread to meet that timing
        if ((startTime.time_since_epoch().count() != 0) &&
            (mInterval.count() != 0))
        {
            endTime = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(mInterval - (endTime - startTime));
        }
    }

    if (mOptions->verbose == true)
    {
        std::cerr << "Stopped data thread." << std::endl;
    }
}