/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Gunnar Ryder for Hellbender, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * gnss_logger.hpp - Session manager for gnss communications
 */

#pragma once

#include <functional>

#include "gps.h"

#include "app_options.hpp"
#include "thread.hpp"

class GnssData : protected Thread
{
public:

    GnssData(AppOptions *opts);
    ~GnssData();	
    
    void SetupGpsdConnect();
    void TeardownGpsdConnect();

    void StartStream();
    void StopStream();
    void SignalGnssLock();

    bool IsFixed();

    typedef std::function<void(gps_data_t&)> DataFunc;
    void SetLogFunc(DataFunc func);

    AppOptions* GetOptions() { return mOptions; }

protected:
    virtual void ThreadFunc() override;
    
    bool            mConnected;
    bool            mStreaming;
    int             mMode;

    gps_data_t      mGpsData;
    DataFunc        mDataFunc;
};
