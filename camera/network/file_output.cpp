/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2022, Chris Niessl, Hellbender Inc.
 *
 * net_output.cpp - send directly to file.
 */
#include <iostream>
#include <iomanip>

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/un.h>
#include <unistd.h>
#include <time.h>
#include <boost/filesystem.hpp>

#include "file_output.hpp"

FileOutput::FileOutput(VideoOptions const *options) : Output(options)
{

  std::vector<std::string> outputLocs;
  std::vector<size_t>      minFreeSizes;
  std::vector<size_t>      maxUsedSizes;
  
  directory_[0] = options_->output;
  directory_[1] = options_->output_2nd;

  minFreeSizes[0] = options_->minfreespace;
  minFreeSizes[1] = options_->minfreespace_2nd;
  
  maxUsedSizes[0] = options_->maxusedspace;
  maxUsedSizes[1] = options_->maxusedspace_2nd;
  
  verbose_ = options_->verbose;
  prefix_  = options_->prefix;

  //TODO - Assume jpeg formate for now. Otherwise extract  
  postfix_ = ".jpg";
  
  int numLocs = 2;
  if(directory_[1] != "")
  {
    numLocs = 1;
  }  
    
  FileManager fileManager_ = FileManager(verbose_, 
                                        prefix_,
                                        minFreeSizes,
                                        maxUsedSizes,
                                        directory_,
                                        numLocs);
}

FileOutput::~FileOutput()
{
}

void FileOutput::outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t /*flags*/)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
      
  try
  {
    tv = getAdjustedTime(timestamp_us);
  }
  catch (std::exception const &e)
  {
    std::cerr << "Time recording issues" << std::endl;

  }
  
  if(directory_[0] != "")
  {
    wrapAndWrite(mem, size, &tv, 0);
  }
  if(directory_[1] != "")
  {  
    wrapAndWrite(mem, size, &tv, 1);
  }
}

struct timeval FileOutput::getAdjustedTime(int64_t timestamp_us)
{
  static bool firstRun = false;
  struct timeval tv;
  time_t   fullSec  = timestamp_us / 1000000;
  long int microSec = timestamp_us % 1000000;
  
  if(!firstRun)
  {
    firstRun = true;
    gettimeofday(&baseTime_, NULL);
    if(baseTime_.tv_usec < microSec)
    {
      baseTime_.tv_usec = 1000000 + baseTime_.tv_usec - microSec;
      baseTime_.tv_sec  = baseTime_.tv_sec - fullSec - 1;
    } else
    {
      baseTime_.tv_usec = baseTime_.tv_usec - microSec;
      baseTime_.tv_sec  = baseTime_.tv_sec - fullSec;
    }
  }

  tv.tv_sec = baseTime_.tv_sec + fullSec;
  tv.tv_usec = baseTime_.tv_usec + microSec;
  if(tv.tv_usec > 1000000)
  {
    tv.tv_usec -= 1000000;
    tv.tv_sec  += 1;
  }
  return tv;
}

void FileOutput::wrapAndWrite(void *mem, size_t size, struct timeval *timestamp, int index)
{

  std::stringstream fileNameGenerator;
  fileNameGenerator << directory_[index];
  fileNameGenerator << prefix_;
  fileNameGenerator << std::setw(10) << std::setfill('0') << timestamp->tv_sec;
  fileNameGenerator << "_";
  fileNameGenerator << std::setw(6) << std::setfill('0') << timestamp->tv_usec;//picCounter;
  fileNameGenerator << postfix_;
  std::string fullFileName = fileNameGenerator.str();
  
  bool fileWritten = false;
  while(!fileWritten)
  {
    if(fileManager_.canWrite(index))
    {
      try
      {
        fileManager_.addFile(index, size, fullFileName);
        writeFile(fullFileName, mem, size, index);
      }
      catch (std::exception const &e)
      {
        std::cerr << "Failed to write file" << std::endl;
      }
      fileWritten = true;
    }
    else
    {
      std::cerr << "Not enough space. Deleting old files and retrying." << std::endl;
    }   
  }

}

void FileOutput::writeFile(std::string fullFileName, void *mem, size_t size, int index)
{
  //open file name and assign fd
  int fd, ret;
  fd = open(fullFileName.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0644);
  if ((ret = write(fd, mem, size)) < 0) {
    throw std::runtime_error("failed to write data");
  }
  close(fd);
  
  if (verbose_)
  {
    std::cerr << "writing " << ret << " bytes to ";
    std::cerr << fullFileName << std::endl;
  }
}
