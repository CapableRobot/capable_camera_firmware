/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2022, Raspberry Pi (Trading) Ltd.
 *
 * net_output.cpp - send output over network.
 */

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>

#include <nlohmann/json.hpp>
#include "net_input.hpp"

#define DEFAULT_PATH "/tmp/config.sock\0"
#define DEFAULT_PATH_SIZE 17

using json = nlohmann::json;

NetInput::NetInput(VideoOptions *options)
{
    options_ = options;

	sock_ = {};
   	sock_.sun_family = AF_UNIX;
   	strncpy(sock_.sun_path, DEFAULT_PATH, DEFAULT_PATH_SIZE);

   	fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd_ < 0) {
		throw std::runtime_error("unable to open unix socket");
	}

	if (connect(fd_, (struct sockaddr *) &sock_, sizeof(struct sockaddr_un)) == -1) {
   		throw std::runtime_error("unable to connect to unix socket");
   	}

    fd_to_check_[0].fd = fd_;
    fd_to_check_[0].events = POLLIN;
}

NetInput::~NetInput()
{
	close(fd_);
}

void NetInput::update_options(uint8_t *buffer)
{
  try
  {
    json new_cfg = json::parse(buffer);
    json recording_cfg = new_cfg.at("recording");
    for(json::iterator ii = recording_cfg.begin(); ii != recording_cfg.end(); ++ii)
    {
      if(ii.key() == "socket")
      {
        options_->output = ii.value();
      }
    }
    
  }catch(json::parse_error& e)
  { 
     std::cout << e.what() << std::endl;
  }
}

size_t NetInput::poll_input()
{
  static uint8_t inbound_buf[2048];
  poll(fd_to_check_, 1, 0);
  if (fd_to_check_[0].revents & POLLIN)
  {
    size_t bytes_in = read(fd_to_check_[0].fd, inbound_buf, 2048);
    if(bytes_in > 0)
    {
      std::cout << "Received config: " << bytes_in << " bytes" << std::endl;
      std::cout << inbound_buf << std::endl;
      
      inbound_buf[bytes_in] = 0;
      update_options(inbound_buf);
      
      return bytes_in;
    }
  }
  return 0;
}

