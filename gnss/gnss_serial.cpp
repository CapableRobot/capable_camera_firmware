/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Chris Niessl for Capable Robot Components, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * gnss_serial.cpp - Session manager for serial communications
 */
 
#include "include/gnss_serial.hpp"
#include "include/serial_options.hpp"

GNSSserial::GNSSserial(SerialOptions *opts)
	: options_(opts)
{
  if (options_->verbose && !options_->help)
  {
    std::cerr << "Created..." << std::endl;
  }
}

GNSSserial::~GNSSserial()
{
  if (options_->verbose && !options_->help)
  {
    std::cerr << "Closing GNSS Serial" << std::endl;
  }
  StopLog();
  TeardownSerialConenct();
}

void GNSSserial::SetupSerialConenct()
{
  if (options_->verbose)
  {
    std::cerr << "Opening serial..." << std::endl;
  }

  //TODO
  return;
}

void GNSSserial::TeardownSerialConenct()
{
  if (options_->verbose)
  {
    std::cerr << "Closing serial..." << std::endl;
  }
  //TODO
  return;
}

void GNSSserial::StartLog()
{
  if (options_->verbose)
  {
    std::cerr << "Starting log..." << std::endl;
  }
  //TODO
  return;
}

void GNSSserial::StopLog()
{
  if (options_->verbose)
  {
    std::cerr << "Stopping log..." << std::endl;
  }
  //TODO
  return;
}

void GNSSserial::SignalGNSSLock()
{
  if (options_->verbose)
  {
    std::cerr << "Passing on GNSS Lock..." << std::endl;
  }
  //TODO
  return;
}
