/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Chris Niessl for Capable Robot Components, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * app.cpp - Main entry point for GNSS over Serial UART communicator
 */

#include <exception>
#include <iostream>

#include <signal.h>

#include "gnss_data.hpp"
#include "gnss_logger.hpp"
#include "app_options.hpp"

bool doExit = false;

void SigHandle(int sigNum)
{
    if (sigNum == SIGINT)
    {
        doExit = true;
    }
}

int main(int argc, char *argv[])
{
    bool optionsValid = false;
    AppOptions options; 
            
    // Handle parsing input arguments
    try
    {
        if (options.Parse(argc, argv))
        {
            if (options.verbose)
            {
                options.Print();
            }
            optionsValid = true;
        }
    }
    catch (std::exception const &e)
    {
        std::cerr << "ERROR: *** " << e.what() << " ***" << std::endl;
        return -1;
    }
    
    // If the options are valid, continue with the application
    if(optionsValid)
    {
        signal(SIGINT, &SigHandle);

        // Setup objects
        GnssData data(&options);
        GnssLogger logger(&options);

        // Setup connection/stream and prepare for data handling
        data.SetupGpsdConnect();
        data.StartStream();
        data.SetLogFunc([&logger](gps_data_t &data) { logger.AddData(data); });

        // Start logging
        logger.Start();

        // Continue until exit signal received
        while (doExit == false)
        {
            
        }

        logger.Stop();
        data.TeardownGpsdConnect();
    }
    
    return 0;
}
