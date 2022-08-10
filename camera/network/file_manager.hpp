/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2022, Chris Niessl, Hellbender Inc.
 *
 * file_output.hpp - send directly to file
 */

#pragma once

#include <netinet/in.h>
#include <sys/un.h>
#include <sys/time.h>

#include <queue>
#include "output.hpp"

typedef std::pair<size_t, std::string> fileInfo; 
typedef std::pair<std::time_t, fileInfo> filePoint;

class FileOutput : public Output
{
public:
    FileOutput(VideoOptions const *options);
    ~FileOutput();

protected:

    void accountForExistingFiles(int index);
    bool checkAndFreeSpace(int index);
    void writeFile(std::string fullFileName, void *mem, size_t size, int index);
    void deleteOldestFile(int index);


private:

    static const int NUM_MAX_DESTS = 2;

    bool verbose_;
    std::string directory_[2];
    std::string prefix_;
    std::string postfix_;

    std::queue<std::string> filenameQueue_[NUM_MAX_DESTS];
    std::queue<size_t>      filesizeQueue_[NUM_MAX_DESTS];

    std::priority_queue<filePoint, std::vector<filePoint>, std::greater<filePoint>> oldFileQueue_[NUM_MAX_DESTS];
    
    size_t minFreeSizeThresh_[NUM_MAX_DESTS];
    size_t maxUsedSizeThresh_[NUM_MAX_DESTS];
    size_t currentUsedSize_[NUM_MAX_DESTS];
    
};
