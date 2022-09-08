/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Gunnar Ryder for Hellbender, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * app_options.cpp - Options parser for gnss logger communication
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
  bool logSnr;
  int debugLevel;
  int maxSize;
  int minMode;
  int logDuration;
  std::string path;
  std::string readyPath;
  std::string ext;
  std::string config_file;

  virtual bool JSONParse();

  virtual bool Parse(int argc, char *argv[]);
  virtual void Print() const;

protected:
  boost::program_options::options_description mOptions;

};
