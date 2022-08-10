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

FileManager::FileManager(VideoOptions const *options) :
    ,filenameQueue_()
    ,filesizeQueue_()
    ,oldFileQueue_()
{
  prefix_   = prefix;
  postfix_  = ".jpg"; //postfix;
  verbose_  = verbose;
  canWrite_ = true;
  
  for(int ii = 0; ii < recordLocs_; ii += 1)
  {
    directory_[ii] = output[ii];
    minFreeSizeThresh_[ii] = minFreeSizeThresh[ii];
    maxUsedSiseThresh_[ii] = maxUsedSizeThresh[ii];
    accountForExistingFiles(ii);
  }

  directory_[0] = options_->output;
  directory_[1] = options_->output_2nd;
  prefix_ = options_->prefix;
  	//std::mutex encode_mutex_;
}

FileManager::~FileManager()
{
}

void FileManager::accountForExistingFiles(int index)
{
    using namespace boost::filesystem;
    try
    {
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
    catch (std::exception const &e)
    {
      std::cerr << "Error scanning directory: " << directory_[index];
      std::cerr << ". Not using it" << std::endl;
      directory_[index] = "";
    }
}

bool FileManager::canWrite(int index)
{
  return canWrite_[index];
}

bool FileManager::checkFreeSpace(int index)
{
  bool freeSpaceAvail = true;
  boost::filesystem::space_info freeSpace = boost::filesystem::space(directory_[index]);
  std::unique_lock<std::mutex> lock(metric_mutex_);
  if(verbose_)
  {
    std::cout << "Bytes available:" << freeSpace.available << std::endl;
    std::cout << "Bytes used:" << currentUsedSize_[index] << std::endl;
  }
  if(currentUsedSize_[index] > maxUsedSizeThresh_[index] && 
     maxUsedSizeThresh_[index] > 0)
  {
    freeSpaceAvail = false;
  }
  if(freeSpace.available < minFreeSizeThresh_[index] &&
  minFreeSizeThresh_[index] > 0)
  {
    freeSpaceAvail = false;
  }  
  canWrite_[index] = freeSpaceAvail;
  return freeSpaceAvail;
}

void FileManager::deleteThread()
{
  while(true)
  {
    std::unique_lock<std::mutex> lock(encode_mutex_);
    for(int ii = 0; ii < recordLocs_; ii +=1)
    {
      if(!checkFreeSpace(ii))
      {
        deleteOldestFile(ii);
      }
    }
    
  }
}

bool FileManager::checkAndFreeSpace(int index)
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

void FileManager::addFile(int index)
{
  currentUsedSize_[index] += size;
  filesizeQueue_[index].push(size);
  filenameQueue_[index].push(fullFileName);
}

void FileManager::deleteOldestFile(int index)
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

