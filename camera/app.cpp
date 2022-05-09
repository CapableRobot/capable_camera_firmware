/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright 2021 Chris Osterwood for Capable Robot Components, Inc.
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 * 
 * Based on raspberrypi/libcamera-apps/libcamera_vid.cpp - libcamera video record app.
 */

#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>

#include <iomanip>

#include "core/libcamera_encoder.hpp"
#include "output/output.hpp"

using namespace std::placeholders;

// Some keypress/signal handling.

static int signal_received;
static void default_signal_handler(int signal_number)
{
  signal_received = signal_number;
  std::cout << "Received signal " << signal_number << std::endl;
}

static int get_key_or_signal(VideoOptions const *options, pollfd p[1])
{
  int key = 0;
  if (options->keypress)
  {
    poll(p, 1, 0);
    if (p[0].revents & POLLIN)
    {
      char *user_string = nullptr;
      size_t len;
      [[maybe_unused]] size_t r = getline(&user_string, &len, stdin);
      key = user_string[0];
    }
  }
  if (options->signal)
  {
    if (signal_received == SIGUSR1)
      key = '\n';
    else if (signal_received == SIGUSR2)
      key = 'x';
  }
  return key;
}

// The main even loop for the application.

static void execute_stream(LibcameraEncoder &app, VideoOptions const *options)
{

  std::unique_ptr<Output> output = std::unique_ptr<Output>(Output::Create(options));
  app.SetEncodeOutputReadyCallback(std::bind(&Output::OutputReady, output.get(), _1, _2, _3, _4));
  app.StartEncoder();

  app.OpenCamera();
  app.ConfigureVideo();
  app.StartCamera();

  // Monitoring for keypresses and signals.
  signal(SIGUSR1, default_signal_handler);
  signal(SIGUSR2, default_signal_handler);
  pollfd p[1] = { { STDIN_FILENO, POLLIN, 0 } };

  auto start_time = std::chrono::high_resolution_clock::now();
  auto last_time = std::chrono::high_resolution_clock::now();

  for (unsigned int count = 0; ; count++)
  {
    LibcameraEncoder::Msg msg = app.Wait();
    if (msg.type == LibcameraEncoder::MsgType::Quit)
      return;
    else if (msg.type != LibcameraEncoder::MsgType::RequestComplete)
      throw std::runtime_error("unrecognised message!");
    int key = get_key_or_signal(options, p);
    if (key == '\n')
      output->Signal();

    auto this_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = this_time - last_time;
    std::chrono::duration<double> elapsed = this_time - start_time;

    std::cout << "Frame " << std::setw(6) << count << " delta " << diff.count() << std::endl;
    last_time = this_time;

    auto now = std::chrono::high_resolution_clock::now();
    if ((options->timeout && now - start_time > std::chrono::milliseconds(options->timeout)) || key == 'x' ||
      key == 'X')
    {
      app.StopCamera(); // stop complains if encoder very slow to close
      app.StopEncoder();
      return;
    }

    CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
    app.EncodeBuffer(completed_request, app.VideoStream());
  }
}

static void setup_net_options()
{
    options->ParseNet();
}

static void listen_options(VideoOptions *options, bool *end_exec)
{

}

int main(int argc, char *argv[])
{
  bool optionsValid = false;
  bool setupNetCfg = false;
  bool end_exec = true;
  try
  {
    LibcameraEncoder app;
    VideoOptions *options = app.GetOptions();
    if (options->Parse(argc, argv))
    {
      if (options->verbose)
      {
        options->Print();
      }
      setupNetCfg = options->netConfig;
      optionsValid = true;
    }
    
    if(setupNetCfg)
    {
      setup_net_options();
      end_exec = false;
    }
    
    do{
      if(setupNetCfg)
      {
        listen_options(options, end_exec);
      }
      if(optionsValid)
      {
        execute_stream(app, options);
      }
    } while(!end_exec);
    
    if(setupNetCfg)
    {
      //TODO: teardown socket here
    }
    
  }
  catch (std::exception const &e)
  {
    std::cerr << "ERROR: *** " << e.what() << " ***" << std::endl;
    return -1;
  }
  return 0;
}