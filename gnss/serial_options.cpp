/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Chris Niessl for Capable Robot Components, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * serial_options.cpp - Options parser for serial communication
 */

//TODO
//#include <nlohmann/json.hpp>

#include "include/serial_options.hpp"

bool SerialOptions::JSONParse()
{
    return true;
}

bool SerialOptions::Parse(int argc, char *argv[])
{
  using namespace boost::program_options;
  variables_map vm;
    
  // Read options from the command line
  store(parse_command_line(argc, argv, options_), vm);
  notify(vm);
    
  // Read options from a file if specified
  std::ifstream ifs(config_file.c_str());
  if (ifs)
  {
    store(parse_config_file(ifs, options_), vm);
    notify(vm);
  }

  if (help)
  {
    std::cout << options_;
    return false;
  }

  if (version)
  {
    std::cout << "GNSS Serial Communicator v0.1" << std::endl;
    return false;
  }

  return true;
}

void SerialOptions::Print() const
{
    std::cout << "Options:" << std::endl;
    std::cout << "    verbose: " << verbose << std::endl;
    if (!config_file.empty())
      std::cout << "    config file: " << config_file << std::endl;
}
