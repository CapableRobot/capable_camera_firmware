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

FileManager::FileManager(bool verbose, 
                         std::string prefix,
                         std::vector<size_t> minFreeSizeThresh,
                         std::vector<size_t> maxUsedSizeThresh,
                         std::vector<std::string> directory,
                         int recordLocs) :
    filenameQueue_(),
    filesizeQueue_(),
    oldFileQueue_()
{
  prefix_   = prefix;
  postfix_  = ".jpg"; //postfix;
  verbose_  = verbose;
  recordLocs_ = recordLocs;
  
  for(int ii = 0; ii < recordLocs_; ii += 1)
  {
    canWrite_[ii] = true;
    currentUsedSize_[ii] = 0;
    directory_[ii] = directory[ii];
    minFreeSizeThresh_[ii] = minFreeSizeThresh[ii];
    maxUsedSizeThresh_[ii] = maxUsedSizeThresh[ii];
    accountForExistingFiles(ii);
  }
 	
  delete_thread_ = std::thread(&FileManager::deleteThread, this);
}

FileManager::~FileManager()
{
    delete_thread_.join();
}

bool FileManager::canWrite(int index)
{
  std::unique_lock<std::mutex> lock(metric_mutex_);
  return canWrite_[index];
}

void FileManager::addFile(int index, size_t size, std::string fullFileName)
{
  std::unique_lock<std::mutex> lock(metric_mutex_);
  currentUsedSize_[index] += size;
  filesizeQueue_[index].push(size);
  filenameQueue_[index].push(fullFileName);
  free_cond_var_.notify_all();
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
            
            {
              //add to queue
              std::unique_lock<std::mutex> lock(metric_mutex_);

              currentUsedSize_[index] += size;
              fileInfo sizeFilePair = std::make_pair(size, current_file);
              filePoint fileToAdd   = std::make_pair(writeTime, sizeFilePair);
              oldFileQueue_[index].push(fileToAdd);
            }
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

void FileManager::deleteThread()
{
  while(true)
  {
    std::unique_lock<std::mutex> lock(metric_mutex_);
    for(int ii = 0; ii < recordLocs_; ii +=1)
    {
      if(!checkFreeSpace(ii))
      {
        deleteOldestFile(ii);
      }
    }
    free_cond_var_.wait_for(lock, 50ms);
  }
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

