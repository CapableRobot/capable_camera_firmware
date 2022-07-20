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
  directory_[0] = options_->output;
  directory_[1] = options_->output_2nd;
  prefix_ = options_->prefix;
  
  //TODO - Assume jpeg formate for now. Otherwise extract 
  postfix_ = ".jpg";
  
  currentUsedSize_[0] = 0;
  currentUsedSize_[1] = 0;
  
  minFreeSizeThresh_[0] = options_->minfreespace;
  maxUsedSizeThresh_[0] = options_->maxusedspace;

  minFreeSizeThresh_[1] = options_->minfreespace_2nd;
  maxUsedSizeThresh_[1] = options_->maxusedspace_2nd;

  verbose_ = options_->verbose;

  //Check free space and mark files in the dest directory 
  //for deletion if we need to...
  accountForExistingFiles(0);
  if(directory_[1] != "")
  {
    accountForExistingFiles(1);
  }
}

FileOutput::~FileOutput()
{
}

void FileOutput::accountForExistingFiles(int index)
{
    using namespace boost::filesystem;
    path writeLocation(directory_[index]);
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
            currentUsedSize_[index] += size;
            fileInfo sizeFilePair = std::make_pair(size, current_file);
            filePoint fileToAdd   = std::make_pair(writeTime, sizeFilePair);
            oldFileQueue_[index].push(fileToAdd);
        }
    }
}

bool FileOutput::checkAndFreeSpace(int index)
{
    bool doDelete  = false;
    bool freeSpaceAvail = false;
    boost::filesystem::space_info freeSpace;
    for(int ii = 0; ii < 8; ii+=1)
    {
      freeSpace = boost::filesystem::space(directory_[index]);
      if(verbose_)
      {
          std::cout << "Bytes available:" << freeSpace.available << std::endl;
          std::cout << "Bytes used:" << currentUsedSize_[index] << std::endl;
      }
      if(currentUsedSize_[index] > maxUsedSizeThresh_[index] && 
         maxUsedSizeThresh_[index] > 0)
      {
        doDelete = true;
      }
    
      if(freeSpace.available < minFreeSizeThresh_[index] &&
         minFreeSizeThresh_[index] > 0)
      {
        doDelete = true;
      }
    
      if(doDelete)
      {
        deleteOldestFile(index);
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

void FileOutput::writeFile(std::string fullFileName, void *mem, size_t size, int index)
{
  //open file name and assign fd
  int fd, ret;
  fd = open(fullFileName.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0644);
  if ((ret = write(fd, mem, size)) < 0) {
    throw std::runtime_error("failed to write data");
  }
  close(fd);
  
  currentUsedSize_[index] += size;
  filesizeQueue_[index].push(size);
  filenameQueue_[index].push(fullFileName);
  
  if (verbose_)
  {
    std::cerr << "wrote " << ret << " bytes to ";
    std::cerr << fullFileName << std::endl;
  }
}

void FileOutput::deleteOldestFile(int index)
{
  if(oldFileQueue_[index].size() > 0)
  {
    int res;
    filePoint popOff = oldFileQueue_[index].top();
    size_t size = popOff.second.first;
    std::string name = popOff.second.second;
    
    res = remove(name.c_str());
    if(res == 0)
    {
      if(verbose_)
      {
          std::cerr << "Deleting " << name << std::endl;
      }
      currentUsedSize_[index] -= size;
    }
    else
    {
      std::cerr << "Error attempting to delete file" << std::endl;
    }  
    oldFileQueue_[index].pop();
  }
  else if(filesizeQueue_[index].size() > 0)
  {
    int res = remove(filenameQueue_[index].front().c_str());
    if(res == 0)
    {
      currentUsedSize_[index] -= filesizeQueue_[index].front();
      filenameQueue_[index].pop();
      filesizeQueue_[index].pop();
    }
    else
    {
      std::cerr << "Error attempting to delete file" << std::endl;
      filenameQueue_[index].pop();
    }    
  }
  
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
  
  if(checkAndFreeSpace(index))
  {
    try
    {
      writeFile(fullFileName, mem, size, index);
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

void FileOutput::outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t /*flags*/)
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  
  if(directory_[0] != "")
  {
    wrapAndWrite(mem, size, &tv, 0);
  }
  if(directory_[1] != "")
  {    
    wrapAndWrite(mem, size, &tv, 1);
  }
}
