/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * options.hpp - common program options
 * still_video.hpp - video capture program options
 */

#pragma once

#include <cstdio>
#include <string>
#include <fstream>
#include <iostream>

#include "boost/program_options.hpp"

#include <libcamera/camera_manager.h>
#include <libcamera/control_ids.h>
#include <libcamera/transform.h>

struct Options
{
  Options() : options_("Valid options are", 120, 80)
  {
    using namespace boost::program_options;
    options_.add_options()
      ("help,h", value<bool>(&help)->default_value(false)->implicit_value(true),
       "Print this help message")
      ("version", value<bool>(&version)->default_value(false)->implicit_value(true),
       "Displays the build version number")
      ("verbose,v", value<bool>(&verbose)->default_value(false)->implicit_value(true),
       "Output extra debug and diagnostics")
      ("config,c", value<std::string>(&config_file)->implicit_value("config.txt"),
       "Read the options from a file. If no filename is specified, default to config.txt. "
       "In case of duplicate options, the ones provided on the command line will be used. "
       "Note that the config file must only contain the long form options.")
      ("netconfig,n", value<bool>(&netconfig)->default_value(false)->implicit_value(true), 
        "Connect to the network endpoint of the camera firmware and listen for configuration "
        "changes over the network")
      ("info-text", value<std::string>(&info_text)->default_value("#%frame (%fps fps) exp %exp ag %ag dg %dg"),
       "Sets the information string on the titlebar. Available values:\n"
       "%frame (frame number)\n%fps (framerate)\n%exp (shutter speed)\n%ag (analogue gain)"
       "\n%dg (digital gain)\n%rg (red colour gain)\n%bg (blue colour gain)"
       "\n%focus (focus FoM value)\n%aelock (AE locked status)")
      ("width", value<unsigned int>(&width)->default_value(0),
       "Set the output image width (0 = use default value)")
      ("height", value<unsigned int>(&height)->default_value(0),
       "Set the output image height (0 = use default value)")
      ("timeout,t", value<uint64_t>(&timeout)->default_value(5000),
       "Time (in ms) for which program runs")
      ("output,o", value<std::string>(&output),
       "Set the output file name")
      ("post-process-file", value<std::string>(&post_process_file),
       "Set the file name for configuring the post-processing")
      ("rawfull", value<bool>(&rawfull)->default_value(false)->implicit_value(true),
       "Force use of full resolution raw frames")
      ("hflip", value<bool>(&hflip_)->default_value(false)->implicit_value(true), "Request a horizontal flip transform")
      ("vflip", value<bool>(&vflip_)->default_value(false)->implicit_value(true), "Request a vertical flip transform")
      ("rotation", value<int>(&rotation_)->default_value(0), "Request an image rotation, 0 or 180")
      ("roi", value<std::string>(&roi)->default_value("0,0,0,0"), "Set region of interest (digital zoom) e.g. 0.25,0.25,0.5,0.5")
      ("shutter", value<float>(&shutter)->default_value(0),
       "Set a fixed shutter speed")
      ("analoggain", value<float>(&gain)->default_value(0),
       "Set a fixed gain value (synonym for 'gain' option)")
      ("gain", value<float>(&gain),
       "Set a fixed gain value")
      ("metering", value<std::string>(&metering)->default_value("centre"),
       "Set the metering mode (centre, spot, average, custom)")
      ("exposure", value<std::string>(&exposure)->default_value("normal"),
       "Set the exposure mode (normal, sport)")
      ("ev", value<float>(&ev)->default_value(0),
       "Set the EV exposure compensation, where 0 = no change")
      ("awb", value<std::string>(&awb)->default_value("auto"),
       "Set the AWB mode (auto, incandescent, tungsten, fluorescent, indoor, daylight, cloudy, custom)")
      ("awbgains", value<std::string>(&awbgains)->default_value("0.0,0.0"),
       "Set explict red and blue gains (disable the automatic AWB algorithm)")
      ("flush", value<bool>(&flush)->default_value(false)->implicit_value(true),
       "Flush output data as soon as possible")
      ("wrap", value<unsigned int>(&wrap)->default_value(0),
       "When writing multiple output files, reset the counter when it reaches this number")
      ("brightness", value<float>(&brightness)->default_value(0),
       "Adjust the brightness of the output images, in the range -1.0 to 1.0")
      ("contrast", value<float>(&contrast)->default_value(1.0),
       "Adjust the contrast of the output image, where 1.0 = normal contrast")
      ("saturation", value<float>(&saturation)->default_value(1.0),
       "Adjust the colour saturation of the output, where 1.0 = normal and 0.0 = greyscale")
      ("sharpness", value<float>(&sharpness)->default_value(1.0),
       "Adjust the sharpness of the output image, where 1.0 = normal sharpening")
      ("framerate", value<float>(&framerate)->default_value(30.0),
       "Set the fixed framerate for preview and video modes")
      ("denoise", value<std::string>(&denoise)->default_value("auto"),
       "Sets the Denoise operating mode: auto, off, cdn_off, cdn_fast, cdn_hq")
      ("viewfinder-width", value<unsigned int>(&viewfinder_width)->default_value(0),
       "Width of viewfinder frames from the camera (distinct from the preview window size")
      ("viewfinder-height", value<unsigned int>(&viewfinder_height)->default_value(0),
       "Height of viewfinder frames from the camera (distinct from the preview window size)")
      ("tuning-file", value<std::string>(&tuning_file)->default_value("-"),
       "Name of camera tuning file to use, omit this option for libcamera default behaviour")
      ("lores-width", value<unsigned int>(&lores_width)->default_value(0),
       "Width of low resolution frames (use 0 to omit low resolution stream")
      ("lores-height", value<unsigned int>(&lores_height)->default_value(0),
       "Height of low resolution frames (use 0 to omit low resolution stream")
      ;
  }

  virtual ~Options() {}

  bool help;
  bool version;
  bool verbose;
  bool netconfig;
  uint64_t timeout; // in ms
  std::string config_file;
  std::string output;
  std::string post_process_file;
  unsigned int width;
  unsigned int height;
  bool rawfull;
  libcamera::Transform transform;
  std::string roi;
  float roi_x, roi_y, roi_width, roi_height;
  float shutter;
  float gain;
  std::string metering;
  int metering_index;
  std::string exposure;
  int exposure_index;
  float ev;
  std::string awb;
  int awb_index;
  std::string awbgains;
  float awb_gain_r;
  float awb_gain_b;
  bool flush;
  unsigned int wrap;
  float brightness;
  float contrast;
  float saturation;
  float sharpness;
  float framerate;
  std::string denoise;
  std::string info_text;
  unsigned int viewfinder_width;
  unsigned int viewfinder_height;
  std::string tuning_file;
  unsigned int lores_width;
  unsigned int lores_height;

  virtual bool JSONParse();

  virtual bool Parse(int argc, char *argv[]);
  virtual void Print() const;

protected:
  boost::program_options::options_description options_;

private:
  bool hflip_;
  bool vflip_;
  int rotation_;
};
