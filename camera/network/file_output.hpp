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

#include "file_manager.hpp"

class FileOutput : public Output
{
public:
    FileOutput(VideoOptions const *options);
    ~FileOutput();

    void checkGPSLock();

protected:

    void outputBuffer(void *mem, size_t size, void* prevMem, size_t prevSize, int64_t timestamp_us, uint32_t flags) override;
    struct timeval getAdjustedTime(int64_t timestamp_us);
    void wrapAndWrite(void *mem, size_t size, struct timeval *timestamp, int index);
    void previewWrapAndWrite(void *mem, size_t size, struct timeval *timestamp, int64_t frameNum);
    void writeFile(std::string fullFileName, void *mem, size_t size);

private:

    bool verbose_;
    bool gpsLockAcq_;
    bool writeTempFile_;
    std::string latestFileName_;
    std::string directory_[3];
    std::string previewDir_;
    std::string gpsReadyDir_;
    std::string prefix_;
    std::string postfix_;
    struct timeval baseTime_;
    FileManager fileManager_;
    
};
