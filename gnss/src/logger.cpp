/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Gunnar Ryder for Hellbender, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * logger.cpp - General json data logging class
 */

#include "logger.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <deque>
#include <string>
#include <sstream>
using namespace std::chrono;

#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include "app_options.hpp"
#include "thread.hpp"

Logger::Logger(AppOptions *opts) : 
    Thread(opts),  mDuration(1min), mQueueIndex(0), mLogOpen(false),
    mTotalLogSize(0), mCurrLogSize(0)
{
    SetInterval(1s);

    std::stringstream path(mOptions->path);
    std::string currPath = "/";
    int status = 0;
    for (std::string token; std::getline(path, token, '/');)
    {
        if (token.empty() == false)
        {
            currPath += token + "/";

            // Check to see if the directory exists
            DIR *dir = opendir(currPath.c_str());
            if (dir != nullptr)
            {
                closedir(dir);
            }
            // The directory doesn't exist, so make it
            else
            {
                status = mkdir(currPath.c_str(), S_IRWXU | S_IRWXG);
                if (status != 0)
                {
                    // Something failed, so fall back to a default (safe) location
                    currPath = "/temp/";
                    mOptions->path = currPath;

                    if (mOptions->verbose)
                    {
                        std::cerr << "Failed to create needed directory." << std::endl;
                        std::cerr << "Falling back to " << currPath << std::endl;
                    }
                    break;
                }
            }
        }
    }

    // Check to make sure there is a final slash on the path, if not add it
    if (mOptions->path.back() != '/')
    {
        mOptions->path += "/";
    }
}
Logger::~Logger() = default;

void Logger::SetFileDuration(seconds &&duration)
{
    // Set the duration internally
    mDuration = duration;
}
std::string Logger::GetDateTimeString(timespec time)
{
    // Set up string for formatting
    const short STR_RESIZE = 128;
    std::string dtString;
    dtString.resize(STR_RESIZE);

    // Convert the time and format it
    std::tm *currTime = gmtime(&time.tv_sec);
    size_t count = strftime(dtString.data(), STR_RESIZE, "%FT%T", currTime);
    dtString.resize(count);

    // Setup milliseconds and append them
    char msAppend[6];
    sprintf(msAppend, ".%03ldZ", (time.tv_nsec / 1000000));
    dtString += msAppend;

    return dtString;
}

void Logger::OpenLog()
{
    // Get time and create date string
    timespec time;
    timespec_get(&time, TIME_UTC);
    mFileName = GetDateTimeString(time) + "." + mOptions->ext;

    // Create full path
    std::string fullPath = mOptions->path + mFileName;

    if (mOptions->verbose == true)
    {
        std::cerr << "Opening log file: " << fullPath << std::endl;
    }

    // Open log file and check that it opened successfullt
    mLogFile.open(fullPath, std::ios_base::trunc | std::ios_base::out);
    mLogOpen = mLogFile.is_open();
    if (mLogOpen == true)
    {
        // Log the open time and add an element to the tracking queue
        mLogOpenTime = steady_clock::now();
        mLogFileQueue.push_back({
            mFileName,
            0,
            time.tv_sec
        });

        // Reset members for the new log
        mOutput = json::array();
        mCurrLogSize = 0;
    }

    if (mOptions->verbose == true)
    {
        std::cerr << "Log file status: " << 
            ((mLogOpen == true) ? "Open" : "Error") << 
            std::endl;
    }
}

void Logger::CheckLogStatus()
{
    // Determine if elapsed time
    steady_clock::time_point currTime = steady_clock::now();
    auto diff = currTime - mLogOpenTime;

    // If there is no file open or the duration has elapsed
    if ((mLogOpen == false) || (diff > mDuration))
    {
        // If there's already a log open, close it
        if (mLogOpen == true)
        {
            mLogFile.close();

            if (mOptions->verbose)
            {
                std::cerr << "Closing log \"" << mFileName << "\"" << std::endl;
            }
        }

        // Handle opening the new log
        OpenLog();
    }

    // Rotate out old entries
    RotateLogs();
}

void Logger::GetLogData()
{
    // Open directory to information about files in it
    DIR *logDir = opendir(mOptions->path.c_str());

    // If the directory opened appropriately
    if (logDir != nullptr)
    {
        struct dirent *currItem = nullptr;
        struct stat fileStat = {0};
        do
        {
            // Get the next item in the directory
            currItem = readdir(logDir);

            // If we got a valid item and the item is a regular file
            if ((currItem != nullptr) && (currItem->d_type == DT_REG))
            {
                // Store the item name
                std::string currItemName = currItem->d_name;

                // Determine if the file has the appropriate extension
                size_t findPos = currItemName.find_last_of(mOptions->ext);
                if ((findPos != std::string::npos) &&
                    (findPos == (currItemName.size() - 1)))
                {
                    // Get the full path of the file and get stats on it
                    std::string currFilePath = mOptions->path + 
                        currItemName;
                    stat(currFilePath.c_str(), &fileStat);

                    // Add the file to the information we know about
                    mLogFileQueue.push_back({
                        currItemName,
                        fileStat.st_size,
                        fileStat.st_mtime
                    });

                    if (mOptions->verbose == true)
                    {
                        std::cerr << "Tracking log file " << currItemName <<
                            std::endl;
                    }

                    // Accumulate the size of that log file
                    mTotalLogSize += fileStat.st_size;
                }
            }
        } while (currItem != nullptr);

        // Sort the information by oldest time first
        std::sort(mLogFileQueue.begin(), mLogFileQueue.end(),
            [](FileData &a, FileData &b) {
                return a.epoch < b.epoch;
        });
    }
    closedir(logDir);
}

void Logger::RotateLogs()
{
    // If we don't have any data accumulated, check to see if there is any
    // in the output directory
    if (mTotalLogSize == 0)
    {
        GetLogData();
    }

    if (mOptions->verbose == true)
    {
        std::cerr << "Total log size: " << (mTotalLogSize / 1000) <<
            "kB"<< std::endl;
        std::cerr << "Config size: " << mOptions->logSize << "kB"<< std::endl;
    }

    // Remove logs until we're smaller than the limit or we only have one file
    while (((mTotalLogSize / 1000) >= mOptions->logSize) &&
        (mLogFileQueue.size() != 1))
    {
        // Get the full path of the front item
        FileData &frontData = mLogFileQueue.front();
        std::string fullPath = mOptions->path + frontData.name;

        // Remove the file and remove the size from the running total
        unlink(fullPath.c_str());
        mTotalLogSize -= frontData.size;

        if (mOptions->verbose == true)
        {
            std::cerr << "Removing log file " << frontData.name << std::endl;
        }
        
        mLogFileQueue.pop_front();
    }
}

void Logger::QueueData(json &data)
{
    // Add data to the queue
    mDataQueue[mQueueIndex].push(data);
}

void Logger::ProcessData(short queueIndex)
{
    // Remove the current file size from the total
    mTotalLogSize -= mCurrLogSize;

    if ((mOptions->verbose == true) && (mOptions->debugLevel > 0))
    {
        std::cerr << "Writing data to file:" << std::endl;
    }

    std::queue<json> &dataQueue = mDataQueue[queueIndex];
    while (dataQueue.empty() == false)
    {
        if ((mOptions->verbose == true) && (mOptions->debugLevel > 0))
        {
            std::cerr << dataQueue.front() << std::endl;
        }

        // Add the front item to the output object and pop it from the queue
        mOutput += dataQueue.front();
        dataQueue.pop();
    }

    // Rewrite contents of the file
    mLogFile.seekp(0);
    mLogFile << mOutput.dump(1, '\t', true) << std::endl;
    mLogFile.flush();

    // Update current log size and add it to the total
    mCurrLogSize = mLogFile.tellp();
    mLogFileQueue.back().size = mCurrLogSize;
    mTotalLogSize += mCurrLogSize;
}

void Logger::ThreadFunc()
{
    // Change the queue that data is added to
    short queueIndex = mQueueIndex;
    mQueueIndex ^= 1;

    // Check to see if a new file needs created
    CheckLogStatus();

    // If a log is open output the data
    if (mLogOpen == true)
    {
        ProcessData(queueIndex);
    }
}