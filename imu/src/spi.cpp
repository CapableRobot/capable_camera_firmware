/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2022 Chris Niessl for Hellbender, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * spi.cpp - SPI wrapper for IMU Userspace driver
 */

#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>

#include <signal.h>
#include <unistd.h>

#include "spi.hpp"

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/ioctl.h>

IMU_SPI_Wrapper:IMU_SPI_Wrapper()
{
  default_speed = 1000000;
}

int IMU_SPI_Wrapper::transfer(uint8_t *out_buffer, 
                               uint8_t *in_buffer, 
                               uint32_t size)
{
  struct spi_ioc_transfer tfer;
  uint64_t ob = reinterpret_cast<uint64_t>(out_buffer);
  uint64_t ib = reinterpret_cast<uint64_t>(in_buffer);
  generate_spi_ioc(tfer, ob, ib, size, default_speed);
  
  return ioctl(current_fd, IOC_MESSAGE, &tfer); 
}

i IMU_SPI_Wrapper::write(uint8_t *out_buffer, uint32_t len)
{
  uint32_t written = 0;
  struct spi_ioc_transfer tfer;
  if(len < SPI_DEFAULT_CHUNK_SIZE)
  {
    
  
  }else{
    std::err << "SPI: Writing size exceeds buffer size" << std::endl;
    
  }
  
  return written;
}

uint32_t IMU_SPI_Wrapper::readByte(uint8_t *in_buffer)
{
  struct spi_ioc_transfer tfer;
}

uint32_t IMU_SPI_Wrapper::readInto(uint8_t *in_buffer, uint32_t size)
{
  struct spi_ioc_transfer tfer;
}

void IMU_SPI_Wrapper::generate_spi_ioc(struct spi_ioc_transfer &spi_ioc_to_use,
                                       uint64_t tx_buf,
                                       uint64_t rx_buf,
                                       uint32_t len,
                                       uint32_t speed_hz,);
{
  spi_ioc_to_use.tx_buf = tx_buf;
  spi_ioc_to_use.rx_bug = rx_buf;
  spi_ioc_to_use.len    = len;
  spi_ioc_to_use.speed_hz = speed_hz;

  spi_ioc_to_use.delay_usecs = 10;
  spi_ioc_to_use.bits_per_word = 8;
  spi_ioc_to_use.cs_change = 0;
  spi_ioc_to_use.tx_nbits = 0;
  spi_ioc_to_use.rx_bits  = 0;
  spi_ioc_to_use.pad = 0;
}
