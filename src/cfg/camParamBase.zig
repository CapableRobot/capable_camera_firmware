// Copyright 2021 Chris Niessl for Capable Robot Components, Inc.
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
const mem = std.mem;
const fmt = std.fmt;

const imgCfg = @import("MutImgCfg.zig");

const fullFilePath: []const u8 = "src/cfg/MutImgCfg.zig";

const imgCfgStctQuineStr: []const u8 = 
\\pub const MutableImgCfg = struct {
\\    hpx: u16,
\\    vpx: u16,
\\    fps: u8
\\};
\\
;

const imgCfgVarQuineOpen: []const u8 =
\\pub const mutableImgCfg = MutableImgCfg {
\\
;

const imgCfgVarQuineStr: []const u8  = 
\\    .hpx = {},
\\    .vpx = {},
\\    .fps = {}
\\
;

const imgCfgVarQuineClose: []const u8 =
\\};
;

pub fn write_out_cam(cfg_filename: []const u8,
                     cfg_params:   MutableImgCfg)
                anyerror!void{                 
    
    const output_file = try std.fs.cwd().createFile(
        cfg_filename, .{ .read = true });
    defer output_file.close();
    
    var quineBuff: [256]u8 = undefined;
    const outputSlice = quineBuff[0..];
    
    const filledStr = try fmt.bufPrint(outputSlice, imgCfgVarQuineStr, 
        .{cfg_params.hpx, cfg_params.vpx, cfg_params.fps});
    
    try output_file.writeAll(imgCfgStctQuineStr);
    try output_file.writeAll(imgCfgVarQuineOpen);
    try output_file.writeAll(filledStr);
    try output_file.writeAll(imgCfgVarQuineClose);    
    return;
}
