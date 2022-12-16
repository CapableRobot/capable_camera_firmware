/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2022, Chris Niessl, Hellbender Inc.
 *
 * file_output.cpp - send directly to file.
 */
#include <iostream>
#include <iomanip>
#include <chrono>

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/un.h>
#include <unistd.h>
#include <time.h>
#include <boost/filesystem.hpp>

#include "file_output.hpp"

FileOutput::FileOutput(VideoOptions const *options) : Output(options),
                                                      queue_mutex_(),
                                                      queue_notify_(),
                                                      writeTaskQueue_()
{

  std::vector<size_t> minFreeSizes = {0, 0};
  std::vector<size_t> maxUsedSizes = {0, 0};
  
  directory_[0] = options_->output;
  directory_[1] = options_->output_2nd;

  previewDir_   = options_->previewStreamDir;
  gpsReadyDir_  = options_->gpsLockCheckDir;

  minFreeSizes[0] = options_->minfreespace;
  minFreeSizes[1] = options_->minfreespace_2nd;
  
  std::cerr << "Initializing sizes.." << std::endl;
  
  maxUsedSizes[0] = options_->maxusedspace;
  maxUsedSizes[1] = options_->maxusedspace_2nd;
  
  verbose_ = options_->verbose;
  prefix_  = options_->prefix;
  writeTempFile_ = options_->writeTmp;

  //TODO - Assume jpeg format for now. Otherwise extract
  postfix_ = ".jpg";
  int numLocs = 2;
  gpsLockAcq_ = false;

  //Check if directories exist, and if not then ignore them 
  if(!boost::filesystem::exists(directory_[0]))
  {
    directory_[0] = "";
  }
  if(!boost::filesystem::exists(directory_[1]))
  {
    directory_[1] = "";
  }
  if(!boost::filesystem::exists(previewDir_))
  {
    previewDir_ = "";
  }

  //TODO parameterize this
  lastImageWrittenFile_ = "/tmp/lastRecordedImage.txt";
  //framebufferSizeFile_ = TODO;

  std::cerr << "Initializing file handler..." << std::endl;
  fileManager_.initVars(verbose_,
                        prefix_,
                        minFreeSizes,
                        maxUsedSizes,
                        directory_,
                        numLocs);

  writer_thread_ = std::thread(&FileOutput::writerThread, this);
}

FileOutput::~FileOutput()
{
}

void FileOutput::checkGPSLock()
{
  if(boost::filesystem::exists(gpsReadyFile_))
  {
    gpsLockAcq_ = true;
  }
}

void FileOutput::outputBuffer(void *mem,
                              size_t size,
                              void *prevMem,
                              size_t prevSize,
                              int64_t timestamp_us,
                              uint32_t /*flags*/)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  static int32_t frameNumTrun = 0;

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
    if(gpsReadyFile_ == "" || gpsLockAcq_)
    {
      wrapAndWrite(mem, size, &tv, 1);
    }
  }
  if(previewDir_ != "")
  {
    previewWrapAndWrite(prevMem, prevSize, frameNumTrun);
  }

  frameNumTrun = (frameNumTrun + 1) % 1000;
  if((frameNumTrun % 100 == 0) && (gpsReadyFile_ != "")) {
    checkGPSLock();
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
  //Generate the final filename
  std::stringstream fileNameGenerator;
  fileNameGenerator << directory_[index];
  fileNameGenerator << prefix_;
  fileNameGenerator << std::setw(10) << std::setfill('0') << timestamp->tv_sec;
  fileNameGenerator << "_";
  fileNameGenerator << std::setw(6) << std::setfill('0') << timestamp->tv_usec;//picCounter;
  std::string partialName  = fileNameGenerator.str();
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
        writeFile(partialName, mem, size);
      }
      catch (std::exception const &e)
      {
        std::cerr << "Failed to write file" << std::endl;
      }
      fileWritten = true;
    }
  }
  //After file is written, if we are the primary, set the latest marker
  if(index == 0)
  {
    int fd, ret;
    size_t latestSize = fullFileName.size();
    fd = open(lastImageWrittenFile_.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0644);
    if ((ret = write(fd, fullFileName.c_str(), latestSize)) < 0) {
      throw std::runtime_error("failed to write data");
    }
    close(fd);
  }
}

void FileOutput::previewWrapAndWrite(void *mem, size_t size, int64_t frameNum)
{
  std::stringstream fileNameGenerator;
  fileNameGenerator << previewDir_;
  fileNameGenerator << "preview_";
  fileNameGenerator << std::setw(3) << std::setfill('0') << frameNum;
  std::string fullFileName = fileNameGenerator.str();
  try
  {
    writeFile(fullFileName, mem, size);
  }
  catch (std::exception const &e)
  {
    std::cerr << "Failed to write file" << std::endl;
  }

}

void FileOutput::writeFile(std::string partialFileName, void *mem, size_t size)
{
  void* queueDataHold = malloc(size);
  memcpy(queueDataHold, mem, size);
  imageContent sizeBufferPair = std::make_pair(size, queueDataHold);
  imageFileInfo fileToAdd   = std::make_pair(partialFileName, sizeBufferPair);
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    writeTaskQueue_.push(fileToAdd);
    queue_notify_.notify_all();
  };
}

void FileOutput::writerThread()
{
  bool keep_alive = true;
  bool process_pic = false;
  imageFileInfo fileToWrite;
  imageContent sizeBufferPair;
  using namespace std::chrono_literals;
  while(keep_alive)
  {
    keep_alive = GetContinueRunningStatus();
    process_pic = false;
    while(!process_pic){
      //CHECK IF QUEUE IS LOADED
      std::unique_lock <std::mutex> lock(queue_mutex_);
      if (writeTaskQueue_.size() > 0) {
        //PROCESS ENTRY
        fileToWrite = writeTaskQueue_.front();
        sizeBufferPair = fileToWrite.second;
        process_pic = true;
        if(writeTaskQueue_.size() > 10) {
          std::cout << "Queue size:" << writeTaskQueue_.size() << std::endl;
        }
      }else{
        queue_notify_.wait_for(lock, 200ms);
      }
    }
    if(process_pic)
    {
      int fd, ret;
      bool writeSuccess = true;
      std::string name = fileToWrite.first;
      std::string tmpName = name + ".tmp";
      std::string finalName = name + postfix_;
      size_t size = sizeBufferPair.first;
      void* mem   = sizeBufferPair.second;

      if(writeTempFile_)
      {
        fd = open(tmpName.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if ((ret = write(fd, mem, size)) < 0)
        {
          writeSuccess = false;
        }
        close(fd);
      }
      else
      {
        fd = open(finalName.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if ((ret = write(fd, mem, size)) < 0)
        {
          writeSuccess = false;
        }
        close(fd);
      }
      if(writeSuccess)
      {
        if(writeTempFile_)
        {
          boost::filesystem::rename(tmpName, finalName);
        }
        if (verbose_)
        {
          std::cout << "Writing " << ret << " bytes to ";
          std::cout << fileToWrite.first << std::endl;
        }
        {
          std::unique_lock <std::mutex> lock(queue_mutex_);
          free(mem);
          writeTaskQueue_.pop();
        }
      }
      else if(verbose_)
      {
        std::cerr << "Can't write. Keeping in queue: " << ret << std::endl;
      }
    }
    else
    {
      std::this_thread::yield();
    }
  }
}