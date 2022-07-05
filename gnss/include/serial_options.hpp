/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Chris Niessl for Capable Robot Components, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * serial_options.hpp - Options parser for serial communication
 */

#pragma once

#include <cstdio>
#include <string>
#include <fstream>
#include <iostream>

#include "boost/program_options.hpp"

struct SerialOptions
{
  SerialOptions() : options_("Valid options are", 120, 80)
  {
    using namespace boost::program_options;
    options_.add_options()
      ("help,h", value<bool>(&help)->default_value(false)->implicit_value(true),
       "Print this help message")
      ("version", value<bool>(&version)->default_value(false)->implicit_value(true),
       "Displays the build version number")
      ("verbose,v", value<bool>(&verbose)->default_value(false)->implicit_value(true),
       "Output extra debug and diagnostics")
      ("config,c", value<std::string>(&config_file)->implicit_value("config.txt"),
       "Read the options from a file. If no filename is specified, default to config.txt. "
       "In case of duplicate options, the ones provided on the command line will be used. "
       "Note that the config file must only contain the long form options.")
      ;
  }

  virtual ~SerialOptions() {}

  bool help;
  bool version;
  bool verbose;
  std::string config_file;

  virtual bool JSONParse();

  virtual bool Parse(int argc, char *argv[]);
  virtual void Print() const;

protected:
  boost::program_options::options_description options_;

};
