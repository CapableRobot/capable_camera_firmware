/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * net_output.cpp - send output over network.
 */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <time.h>

#include "net_output.hpp"

FileOutput::FileOutput(VideoOptions const *options) : Output(options)
{
  prefix_ = options_->filePrefix;
}

FileOutput::~FileOutput()
{
}

void FileOutput::outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t /*flags*/)
{

  //generate file name
  time_t nowTime;
  time(&nowTime);
  ctime(&nowTime);
  std::stringstream fileNameGenerator;
  fullFileName << prefix << "_" << ctime(&nowTime);
  std::string fullFileName = fullFileName.str();
  
  //open file name and assign fd
  int fd;
  fd = open(fullFileName.c_str(), O_CREAT|O_WRONLY|O_TRUNC);
  
  if ((ret = write(fd, mem, size)) < 0) {
    throw std::runtime_error("failed to send data on unix socket");
  }

  close(fd);

  if (options_->verbose)
  {
    std::cerr << "  wrote " << ret << " bytes\n";
  }
}
