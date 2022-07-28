/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Gunnar Ryder for Hellbender, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * gnss_logger.hpp - Session logger for gnss communications
 */

#pragma once

#include <string>

#include "gps.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include "logger.hpp"

class GnssLogger : public Logger
{
public:

    GnssLogger(bool verbose, int debugLevel, std::string &path, std::string &ext,
        int maxSize, int fileDuration, bool logSnr, int minMode);
    virtual ~GnssLogger();
    
    void AddData(gps_data_t &data);

protected:
    json OrganizeData(gps_data_t &data);

private:
    const bool  mLogSnr;
    const int   mMinMode;

};
