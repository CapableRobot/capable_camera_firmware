/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Chris Niessl for Capable Robot Components, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * gnss_serial.hpp - Session manager for serial communications
 */

#pragma once

#include <sys/mman.h>

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <variant>

#include "serial_options.hpp"

class GNSSserial
{
  public:

  GNSSserial(SerialOptions *opts);
  ~GNSSserial();	
	
  void SetupSerialConenct();
  void TeardownSerialConenct();

  void StartLog();
  void StopLog();
  void SignalGNSSLock();

  SerialOptions* GetOptions() { return options_; }

  protected:
  
  SerialOptions* options_;
  
};
