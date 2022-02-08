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

const camParamBase = @import("cfg/camParamBase.zig");
const imgCfg = @import("cfg/mutImgCfg.zig");

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
    fps: u8 = imgCfg.mutableImgCfg.fps, //10,
    width: u16 = imgCfg.mutableImgCfg.hpx, //4056,
    height: u16 = imgCfg.mutableImgCfg.vpx, //2016,
    quality: u8 = 50,
    codec: Codec = Codec.mjpeg,
};

pub const Config = struct {
    api: Api = Api{},
    recording: Recording = Recording{},
    camera: Camera = Camera{},
    gnss: Gnss = Gnss{},
};

pub fn writeCfg(camera: Camera) void {
    var newCfgParam : imgCfg.MutableImgCfg;
    newCfgParam.hpx = camera.width;
    newCfgParam.vpx = camera.height;
    newCfgParam.fps = camera.fps;
    camParamBase.write_out_cam(camParamBase.fullFilePath,
                               newCfgParam);
}

//add update here, call update launch file
//options.cpp camera core options
// for exposure controls - exposure time alone for now
// white balance setting, gain, libcamera egc
// download libcamera source for rasp-pi
// src/ipa/raspberry-pi - most algorithms in controller/rpi
// AWB, AGC, contrast, black-level
// find out how to expose in libcamera apps
// frame duration limits - usec min and max frame limits


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
