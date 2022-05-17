/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2022, Raspberry Pi (Trading) Ltd.
 *
 * net_input.hpp - receive input over network.
 */

#pragma once

#include <netinet/in.h>
#include <poll.h>
#include <sys/un.h>

#include "output.hpp"

class NetInput
{
public:
    
    NetInput(VideoOptions *options);
    ~NetInput();

    size_t poll_input();
    void   update_options(uint8_t *buffer);

private:
    
    int fd_;
    sockaddr_un sock_;
    struct pollfd fd_to_check_[1];
    
    VideoOptions *options_;
};
