/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2022, Chris Niessl, Hellbender Inc.
 *
 * net_output.cpp - send directly to file.
 */
#include <iostream>
#include <iomanip>


#include <sys/time.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/un.h>
#include <unistd.h>
#include <time.h>
#include <boost/filesystem.hpp>

#include "file_output.hpp"

FileOutput::FileOutput(VideoOptions const *options) : Output(options)
    ,filenameQueue_()
    ,filesizeQueue_()
    ,oldFileQueue_()
{
  directory_ = options_->output;
  prefix_ = options_->prefix;
  
  //TODO - Assume jpeg formate for now. Otherwise extract 
  postfix_ = ".jpg";
  
  currentUsedSize_ = 0;
  
  //TODO - Set these via options
  minFreeSizeThresh_ = options_->minfreespace;
  maxUsedSizeThresh_ = options_->maxusedspace;

  verbose_ = options_->verbose;

  //Check free space and mark files in the dest directory 
  //for deletion if we need to...
  accountForExistingFiles();
}

FileOutput::~FileOutput()
{
}

void FileOutput::accountForExistingFiles()
{
    using namespace boost::filesystem;
    path writeLocation(directory_);
    directory_iterator logDirEnd;

    // cycle through the directory
    for (directory_iterator itr(writeLocation); itr != logDirEnd; ++itr)
    {
        if (is_regular_file(itr->path())) {
            //get info
            std::time_t writeTime = last_write_time(itr->path());
            std::string current_file = itr->path().string();
            size_t size = file_size(current_file);
            
            //display info
            if(verbose_)
            {
                std::cout << "Marking: " << current_file << " size: " << size;
                std::cout << " write time: " << writeTime << std::endl;
            }
            //add to queue
            currentUsedSize_ += size;
            fileInfo sizeFilePair = std::make_pair(size, current_file);
            filePoint fileToAdd   = std::make_pair(writeTime, sizeFilePair);
            oldFileQueue_.push(fileToAdd);
        }
    }
}

bool FileOutput::checkAndFreeSpace()
{
    bool doDelete  = false;
    bool freeSpaceAvail = false;
    boost::filesystem::space_info freeSpace;
    for(int ii = 0; ii < 8; ii+=1)
    {
      freeSpace = boost::filesystem::space("/tmp/recording/");
      if(verbose_)
      {
          std::cout << "Bytes available:" << freeSpace.available << std::endl;
          std::cout << "Bytes used:" << currentUsedSize_ << std::endl;
      }
      if(currentUsedSize_ > maxUsedSizeThresh_)
      {
        doDelete = true;
      }
    
      if(freeSpace.available < minFreeSizeThresh_)
      {
        doDelete = true;
      }
    
      if(doDelete)
      {
        deleteOldestFile();
        doDelete = false;
      }
      else
      {
        freeSpaceAvail = true;
        break;
      }
    }
    
    return freeSpaceAvail;
}

void FileOutput::writeFile(std::string fullFileName, void *mem, size_t size)
{
  //open file name and assign fd
  int fd, ret;
  fd = open(fullFileName.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0644);
  if ((ret = write(fd, mem, size)) < 0) {
    throw std::runtime_error("failed to write data");
  }
  close(fd);
  
  currentUsedSize_ += size;
  filesizeQueue_.push(size);
  filenameQueue_.push(fullFileName);
  
  if (verbose_)
  {
    std::cerr << "  wrote " << ret << " bytes\n";
  }
}

void FileOutput::deleteOldestFile()
{
  if(oldFileQueue_.size() > 0)
  {
    int res;
    filePoint popOff = oldFileQueue_.top();
    size_t size = popOff.second.first;
    std::string name = popOff.second.second;
    
    res = remove(name.c_str());
    if(res == 0)
    {
      if(verbose_)
      {
          std::cerr << "Deleting " << name << std::endl;
      }
      currentUsedSize_ -= size;
    }
    else
    {
      std::cerr << "Error attempting to delete file" << std::endl;
    }  
    oldFileQueue_.pop();
  }
  else if(filesizeQueue_.size() > 0)
  {
    int res = remove(filenameQueue_.front().c_str());
    if(res == 0)
    {
      currentUsedSize_ -= filesizeQueue_.front();
      filenameQueue_.pop();
      filesizeQueue_.pop();
    }
    else
    {
      std::cerr << "Error attempting to delete file" << std::endl;
      filenameQueue_.pop();
    }    
  }
  
}

void FileOutput::outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t /*flags*/)
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  
  std::stringstream fileNameGenerator;
  fileNameGenerator << directory_;
  fileNameGenerator << prefix_;
  fileNameGenerator << std::setw(10) << std::setfill('0') << tv.tv_sec;
  fileNameGenerator << "_";
  fileNameGenerator << std::setw(6) << std::setfill('0') << tv.tv_usec;//picCounter;
  fileNameGenerator << postfix_;
  std::string fullFileName = fileNameGenerator.str();
  
  if(checkAndFreeSpace())
  {
    try
    {
      writeFile(fullFileName, mem, size);
    }
    catch (std::exception const &e)
    {
      std::cerr << "Failed to write file" << std::endl;
    }
  }
  else
  {
    std::cerr << "Not enough space. Deleting old files and retrying." << std::endl;
  }
}
