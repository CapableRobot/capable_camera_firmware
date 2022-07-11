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

struct AppOptions
{
  AppOptions();
  virtual ~AppOptions();

  bool help;
  bool version;
  bool verbose;
  int logSize;
  std::string path;
  std::string ext;
  std::string config_file;

  virtual bool JSONParse();

  virtual bool Parse(int argc, char *argv[]);
  virtual void Print() const;

protected:
  boost::program_options::options_description mOptions;

};
