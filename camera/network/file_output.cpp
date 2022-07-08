/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * net_output.cpp - send output over network.
 */

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/un.h>
#include <unistd.h>
#include <time.h>

#include "file_output.hpp"

FileOutput::FileOutput(VideoOptions const *options) : Output(options)
{
  prefix_ = options_->output;
  //TODO - Assume jpeg formate for now. Otherwise extract 
  postfix_ = ".jpg";
}

FileOutput::~FileOutput()
{
}

void FileOutput::outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t /*flags*/)
{
  static uint32_t picCounter = 0;

  //generate file name
  //time_t nowTime;
  //time(&nowTime);
  //ctime(&nowTime);
  std::stringstream fileNameGenerator;
  fileNameGenerator << prefix_ << "_";
  fileNameGenerator << setw(8) << setfill('0') << picCounter;
  fileNameGenerator << postfix_;
  std::string fullFileName = fileNameGenerator.str();
  
  //open file name and assign fd
  int fd, ret;
  fd = open(fullFileName.c_str(), O_CREAT|O_WRONLY|O_TRUNC);
  
  if ((ret = write(fd, mem, size)) < 0) {
    throw std::runtime_error("failed to write data");
  }

  close(fd);
  
  picCounter += 1;
  if (options_->verbose)
  {
    std::cerr << "  wrote " << ret << " bytes\n";
  }
}
