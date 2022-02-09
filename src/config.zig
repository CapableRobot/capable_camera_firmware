// Copyright 2021 Chris Osterwood for Capable Robot Components, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

const std = @import("std");
const fs = std.fs;
const mem = @import("std").mem;

const imgCfg = @import("cfg/camParamBase.zig");

pub const Api = struct {
    port: u16 = 5000,
};

pub const Gnss = struct {
    debug_period: u16 = 0,
    reset_on_start: bool = true,
};

pub const Recording = struct {
    dir: []const u8 = "/tmp/recording",
    max_size: u64 = 100, // MB
    cleanup_period: u16 = 10, // seconds
    socket: []const u8 = "/tmp/bridge.sock",
};

pub const Codec = enum { mjpeg, h264 };

pub const Camera = struct {
    fps: u8 = 10,
    width: u16 = 4056,
    height: u16 = 2016,
    quality: u8 = 50,
    codec: Codec = Codec.mjpeg,
};

pub const Config = struct {
    api: Api = Api{},
    recording: Recording = Recording{},
    camera: Camera = Camera{},
    gnss: Gnss = Gnss{},
};

//add update here, call update launch file
//options.cpp camera core options
// for exposure controls - exposure time alone for now
// white balance setting, gain, libcamera egc
// download libcamera source for rasp-pi
// src/ipa/raspberry-pi - most algorithms in controller/rpi
// AWB, AGC, contrast, black-level
// find out how to expose in libcamera apps
// frame duration limits - usec min and max frame limits

pub fn validateCamCfg(cfg_params: Camera) bool{
    var isGood: bool = true;
    if (cfg_params.width  == 0){ isGood = false; }
    if (cfg_params.height == 0){ isGood = false; }
    if (cfg_params.fps    == 0){ isGood = false; }
    if (cfg_params.width  > 4096){ isGood = false; }
    if (cfg_params.height > 2048){ isGood = false; }
    if (cfg_params.fps    >   30){ isGood = false; }    
    
    return isGood;
}

pub fn updateCamCfg(reqContent: u8[],
                    cfg_params: &Camera) bool {
    var goodInput = false;
	var contentStream = std.json.TokenStream.init(reqContent);
    cfg_params = try std.json.parse(Camera, &contentStream, .{});
    goodInput = validate(cfg_params);
    if(goodInput){
        try imgCfg.update_script(camParamBase.fullFilePath,
		                         cfg_params.width,
								 cfg_params.height,
								 cfg_params.fps);
		try writeJsonCfg(fullFilePath,
		                 Recording,
		                 cfg_params);
    }
	return goodInput;
}

pub fn load(allocator: *mem.Allocator) Config {
    const max_size = 1024 * 1024;

    const input_file = std.fs.cwd().openFile("config.json", .{}) catch |err| {
        std.log.err("config: failed to open config file\n", .{});
        return Config{};
    };

    const input = input_file.readToEndAlloc(
        allocator,
        max_size,
    ) catch |err| switch (err) {
        error.FileTooBig => {
            std.log.err("config: file too large\n", .{});
            return Config{};
        },
        else => {
            std.log.err("config: file read error\n", .{});
            return Config{};
        },
    };

    var tokens = std.json.TokenStream.init(input);

    return std.json.parse(Config, &tokens, std.json.ParseOptions{
        .allocator = allocator,
        .ignore_unknown_fields = true,
        .allow_trailing_data = true,
    }) catch |err| {
        std.log.err("config: failed to parse config file : {any}\n", .{err});
        return Config{};
    };
}

pub fn writeJsonCfg(allocator: *mem.Allocator) Config {
    const max_size = 1024 * 1024;
	
	const output_file = std.fs.cwd().createFile("config.json", .{.read = true}) catch |err| {
        std.log.err("config: failed to open config file\n", .{});
        return Config{};
    };
	defer output_file.close();
	
	
}