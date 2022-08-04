/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Chris Niessl for Hellbender, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * app_options.cpp - Options parser for imu userspace driver
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

  //App information
  bool help;
  bool verbose;
  
  //Meta configruation
  std::string config_path;  
  std::string config_file;
  
  //SPI BUS configuration values
  std::string SPIdev_path;
  std::string SPIdev_IOC_MESSAGE; //0x40206b00
  uint32_t SPIdev_speed;
  uint8_t  SPIdev_mode;
  
  //SPI IOCTL configuration values
  uint64_t tx_buf;
  uint64_t rx_buf;
  uint32_t len;
  uint32_t speed_hz;
  uint16_t delay_usecs;
  uint8_t  bits_per_word;
  uint8_t  cs_change: u8;
  uint8_t  tx_nbits: u8;
  uint8_t  rx_bits: u8;
  uint16_t pad;

  //Output configuration

  //Configuration Functions
  virtual bool JSONParse();
  virtual bool Parse(int argc, char *argv[]);
  virtual void Print() const;

  //Generation Functions
  virtual void generateIOCTLStruct();

protected:
  boost::program_options::options_description mOptions;

};
