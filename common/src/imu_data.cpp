/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Gunnar Ryder for Hellbender, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * imu_data.cpp - Implementation for IMU data collection class
 */

#include "imu_data.hpp"

#include <chrono>
#include <cstring>
#include <iostream>

ImuData::ImuData(
    ImuPtr &imuPtr,
    unsigned int sampleInterval,
    bool verbose,
    int debugLevel
    ) :
    Thread(verbose, debugLevel),
    mImuPtr(imuPtr)
{
    std::chrono::milliseconds interval{sampleInterval};
    SetInterval(interval);
}

ImuData::~ImuData() {};

void ImuData::SetLogFunc(DataFunc func)
{
    mDataFunc = func;
}

void ImuData::ThreadFunc()
{
    if ((mImuPtr->IsReady() == true) && mDataFunc)
    {
        Data newData;
        memset(&newData, 0, sizeof(Data));
        if (mImuPtr->GetAccelValues(newData.accel) == true)
        {
            newData.status |= DataStatus::AccelAvailable;
        }

        if (mImuPtr->GetGyroValues(newData.gyro) == true)
        {
            newData.status |= DataStatus::GyroAvailable;
        }

        if (mImuPtr->GetMagValues(newData.mag) == true)
        {
            newData.status |= DataStatus::MagAvailable;
        }

        if (mImuPtr->GetTempValue(newData.temp) == true)
        {
            newData.status |= DataStatus::TempAvailable;
        }

        timespec_get(&newData.time, TIME_UTC);
        mDataFunc(newData);
    }
    else
    {
        if (mVerbose == true)
        {
            std::cout << "Imu device is not ready..." << std::endl;
        }
    }
}