/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Gunnar Ryder for Hellbender, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * imu_logger.hpp - Implementation for IMU logging class
 */

#include "imu_logger.hpp"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include "imu_data.hpp"

ImuLogger::ImuLogger(
    std::string &path,
    std::string &ext,
    int maxSize,
    int fileDuration,
    bool verbose,
    int debugLevel
    ) :
    Logger(path, ext, maxSize, fileDuration, verbose, debugLevel) {};

ImuLogger::~ImuLogger() = default;

void ImuLogger::AddData(ImuData::Data &data)
{
    json organizedData = OrganizeData(data);
    QueueData(organizedData);
}

json ImuLogger::OrganizeData(ImuData::Data &data)
{
    json dataObject = json::object();

    std::string key;
    if ((data.status & ImuData::DataStatus::AccelAvailable) > 0)
    {
        key = "accel";
        dataObject[key] = json::object();
        dataObject[key]["x"] = data.accel[0];
        dataObject[key]["y"] = data.accel[1];
        dataObject[key]["z"] = data.accel[2];
    }
    
    if ((data.status & ImuData::DataStatus::GyroAvailable) > 0)
    {
        key = "gyro";
        dataObject[key] = json::object();
        dataObject[key]["x"] = data.gyro[0];
        dataObject[key]["y"] = data.gyro[1];
        dataObject[key]["z"] = data.gyro[2];
    }
    
    if ((data.status & ImuData::DataStatus::MagAvailable) > 0)
    {
        key = "mag";
        dataObject[key] = json::object();
        dataObject[key]["x"] = data.mag[0];
        dataObject[key]["y"] = data.mag[1];
        dataObject[key]["z"] = data.mag[2];
    }
    
    if ((data.status & ImuData::DataStatus::TempAvailable) > 0)
    {
        key = "temp";
        dataObject[key] = data.temp;
    }

    dataObject["time"] = GetDateTimeString(data.time);

    return dataObject;
}