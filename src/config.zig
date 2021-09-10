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
const print = @import("std").debug.print;
const fs = std.fs;
const mem = @import("std").mem;

pub const Api = struct {
    port: u16 = 5000,
};

pub const Config = struct {
    api: Api = Api{},
};

pub fn load(allocator: *mem.Allocator) Config {
    const max_size = 1024 * 1024;

    const input_file = std.fs.cwd().openFile("config.json", .{}) catch |err| {
        print("config: failed to open config file\n", .{});
        return Config{};
    };

    const input = input_file.readToEndAlloc(
        allocator,
        max_size,
    ) catch |err| switch (err) {
        error.FileTooBig => {
            print("config: file too large\n", .{});
            return Config{};
        },
        else => {
            print("config: file read error\n", .{});
            return Config{};
        },
    };

    var tokens = std.json.TokenStream.init(input);

    return std.json.parse(Config, &tokens, std.json.ParseOptions{
        .allocator = allocator,
        .ignore_unknown_fields = true,
        .allow_trailing_data = true,
    }) catch |err| {
        print("config: failed to parse config file : {any}\n", .{err});
        return Config{};
    };
}
