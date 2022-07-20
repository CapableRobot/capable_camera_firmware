/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2022, Chris Niessl, Hellbender Inc.
 *
 * file_output.hpp - send directly to file
 */

#pragma once

#include <netinet/in.h>
#include <sys/un.h>

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

    void wrapAndWrite(void *mem, size_t size, struct timeval *timestamp, int index);
    void outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags) override;

private:
    bool verbose_;
    std::string directory_[2];
    std::string prefix_;
    std::string postfix_;

    std::queue<std::string> filenameQueue_[2];
    std::queue<size_t>      filesizeQueue_[2];

    std::priority_queue<filePoint, std::vector<filePoint>, std::greater<filePoint>> oldFileQueue_[2];
    
    size_t minFreeSizeThresh_[2];
    size_t maxUsedSizeThresh_[2];
    size_t currentUsedSize_[2];
};
