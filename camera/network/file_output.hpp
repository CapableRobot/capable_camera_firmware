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

    void accountForExistingFiles();
    bool checkAndFreeSpace();
    void writeFile(std::string fullFileName, void *mem, size_t size);
    void deleteOldestFile();

    void outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags) override;

private:
    bool verbose_;
    std::string directory_;
    std::string prefix_;
    std::string postfix_;

    std::queue<std::string> filenameQueue_;
    std::queue<size_t>      filesizeQueue_;

    std::priority_queue<filePoint, std::vector<filePoint>, std::greater<filePoint>> oldFileQueue_;
    
    size_t minFreeSizeThresh_;
    size_t maxUsedSizeThresh_;
    size_t currentUsedSize_;
};
