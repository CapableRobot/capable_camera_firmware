// Copyright 2022 Chris Osterwood for Capable Robot Components, Inc.
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
const web = @import("zhp");

const threads = @import("../threads.zig");
const imu = @import("imu.zig");

pub const Sample = struct {
    pub fn get(self: *Sample, request: *web.Request, response: *web.Response) !void {
        try response.headers.append("Content-Type", "application/json");

        const data = threads.imu_ctx.latest();

        try std.json.stringify(data, std.json.StringifyOptions{
            .whitespace = .{ .indent = .{ .Space = 2 } },
        }, response.stream);
    }
};

pub const History = struct {
    pub fn get(self: *History, request: *web.Request, response: *web.Response) !void {
        try response.headers.append("Content-Type", "application/json");

        const data = threads.imu_ctx.history();

        try std.json.stringify(data, std.json.StringifyOptions{
            .whitespace = .{ .indent = .{ .Space = 2 } },
        }, response.stream);
    }
};
