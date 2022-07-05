/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Chris Niessl for Capable Robot Components, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * app.cpp - Main entry point for GNSS over Serial UART communicator
 */

#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>

#include <iomanip>
#include <chrono>
#include <thread>

#include "include/gnss_serial.hpp"
#include "include/serial_options.hpp"

int main(int argc, char *argv[])
{
  bool optionsValid = false;
  SerialOptions *options; 
      
  try
  {
    if (options->Parse(argc, argv))
    {
      if (options->verbose)
      {
        options->Print();
      }
      optionsValid = true;
    }
  }
  catch (std::exception const &e)
  {
    std::cerr << "ERROR: *** " << e.what() << " ***" << std::endl;
    return -1;
  }
  
  if(optionsValid)
  {
    GNSSserial app(options);
  }
  
  std::cout << "Options validated?" << optionsValid << std::endl;
  return 0;
}
