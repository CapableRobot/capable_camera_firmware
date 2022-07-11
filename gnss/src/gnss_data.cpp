/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Gunnar Ryder for Hellbender, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * gnss_data.cpp - Session manager for gnss communications
 */

#include <iostream>
#include <thread>
 
#include "gnss_data.hpp"
#include "app_options.hpp"

#include "gps.h"

#define GPS_WAIT_TIME   1000 // microseconds

GnssData::GnssData(AppOptions *opts) : Thread(opts), mStreaming(false),
    mConnected(false), mLocked(false)
{
    if ((mOptions->verbose == true) && (mOptions->help == false))
    {
        std::cerr << "Created..." << std::endl;
    }
}

GnssData::~GnssData()
{
    if ((mOptions->verbose == true) && (mOptions->help == false))
    {
        std::cerr << "Closing GNSS Serial" << std::endl;
    }
    TeardownGpsdConnect();
}

void GnssData::SetupGpsdConnect()
{
    if (mOptions->verbose == true)
    {
        std::cerr << "Opening gpsd connection..." << std::endl;
    }

    // Use library to open the connection to gpsd
    int status = gps_open("localhost", "2947", &mGpsData);
    if (status == 0)
    {
        mConnected = true;
    }

    if (mOptions->verbose == true)
    {
        std::cerr << "Connection status: " << 
                    ((mConnected == true) ? "success" : "fail") <<
                    std::endl;
    }

    return;
}

void GnssData::TeardownGpsdConnect()
{
    if (mOptions->verbose == true)
    {
        std::cerr << "Closing gpsd connection..." << std::endl;
    }
    
    if (mConnected == true)
    {
        StopStream();

        // Close the connection with gpsd
        gps_close(&mGpsData);
        mConnected = false;
        
        if (mOptions->verbose == true)
        {
            std::cerr << "Closed gpsd connection." << std::endl;
        }
    }

    return;
}

void GnssData::StartStream()
{
    if (mOptions->verbose == true)
    {
        std::cerr << "Starting log..." << std::endl;
    }
    
    // Open the stream if it's not already streaming
    if (mStreaming == false)
    {
        // Start the stream
        int status = gps_stream(&mGpsData, WATCH_ENABLE | WATCH_JSON, NULL);
        if (status == 0)
        {
            // Start the thread to process data
            Start();
            mStreaming = true;
        }

        if (mOptions->verbose == true)
        {
            std::cerr << "Closed gpsd stream." << std::endl;
        }
    }
    return;
}

void GnssData::StopStream()
{
    if (mOptions->verbose == true)
    {
        std::cerr << "Stopping log..." << std::endl;
    }
    
    if (mStreaming == true)
    {
        // Stop the processing thread
        Stop();

        // Stop the stream
        gps_stream(&mGpsData, WATCH_DISABLE, NULL);
        mStreaming = false;

        if (mOptions->verbose == true)
        {
            std::cerr << "Closed gpsd stream." << std::endl;
        }
    }
    return;
}

void GnssData::SignalGnssLock()
{
    if (mOptions->verbose == true)
    {
        std::cerr << "GNSS Lock" << std::endl;
    }
    //TODO
    return;
}
    
void GnssData::SetLogFunc(DataFunc func)
{
    mDataFunc = func;
}

void GnssData::ThreadFunc()
{
    // Wait for data to be ready
    bool ready = gps_waiting(&mGpsData, GPS_WAIT_TIME);

    // If data was received before timeout
    if (ready == true)
    {
        std::string msg;
        char *msgPtr = nullptr;
        int msgSize = 0;

        // If we're in verbose mode, setup output the data we receive
        if (mOptions->verbose == true)
        {
            msg.resize(1024);
            msgPtr = msg.data();
            msgSize = msg.size();
        }

        // Read data and validate the read was successful
        int status = gps_read(&mGpsData, msgPtr, msgSize);
        if (status != -1) {
            if (mOptions->verbose == true)
            {
                // Resize to the amount of data we read
                msg.resize(status);
                std::cout << msg << std::endl;
            }

            // Make sure the mode has been set before 
            if ((MODE_SET & mGpsData.set) != 0)
            {
                // Determine what the lock state is
                bool oldLockState = mLocked;
                switch (mGpsData.fix.mode)
                {
                    case MODE_NOT_SEEN:
                    case MODE_NO_FIX:
                    {
                        if (mLocked == true)
                        {
                            mLocked = false;
                        }
                    }
                    case MODE_2D:
                    case MODE_3D:
                    {
                        if (mLocked == false)
                        {
                            mLocked = true;
                            SignalGnssLock();
                        }
                    }
                }

                if (mOptions->verbose == true && oldLockState != mLocked)
                {
                    std::cerr << "Lock state changed: " <<
                                ((mLocked == true) ? "Locked" : "No Fix") <<
                                std::endl;
                }

                // If there is a lock, pass on the data
                if (mLocked == true)
                {
                    mDataFunc(mGpsData);
                }
            }
        }
    }
}