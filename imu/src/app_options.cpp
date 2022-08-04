/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Chris Niessl for Hellbender, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * app_options.cpp - Options parser for imu userspace driver
 */

#include <fstream>
#include <iostream>
#include <string>

#include "app_options.hpp"

AppOptions::AppOptions() : mOptions("Valid options are", 120, 80)
{
    using namespace boost::program_options;
    mOptions.add_options()
        ("help,h", value<bool>(&help)->default_value(false)->implicit_value(true),
            "Print this help message")
        ("version", value<bool>(&version)->default_value(false)->implicit_value(true),
            "Displays the build version number")
        ("verbose,v", value<bool>(&verbose)->default_value(false)->implicit_value(true),
            "Output extra debug and diagnostics")
        ("config_path,p", value<std::string>(&config_path)->default_value("/tmp/"),
            "Path to look for LED configuration file")
        ("config_file,f", value<std::string>(&config_file)->default_value("imu.json"),
            "LED configuration file")

        ;
}

AppOptions::~AppOptions() = default;

bool AppOptions::JSONParse()
{
    return true;
}

bool AppOptions::Parse(int argc, char *argv[])
{
    using namespace boost::program_options;
    variables_map vm;

    // Read options from the command line
    store(parse_command_line(argc, argv, mOptions), vm);
    notify(vm);

    // Read options from json file if specified
    std::ifstream ifs(configFile.c_str());
    if (ifs)
    {
        store(parse_config_file(ifs, mOptions), vm);
        notify(vm);
    }

    if (help)
    {
        std::cout << mOptions;
        return false;
    }

    if (version)
    {
        std::cout << "IMU Controller v0.1" << std::endl;
        return false;
    }

    return true;
}

void AppOptions::Print() const
{
    std::cout << "Options:" << std::endl;
    std::cout << "    verbose: " << verbose << std::endl;
    if (!configFile.empty())
        std::cout << "    config file: " << configFile << std::endl;
}
