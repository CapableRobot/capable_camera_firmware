// Copyright 2022 Chris Niessl for Capable Robot Components, Inc.
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
const mem = std.mem;
const print = std.debug.print;

const web = @import("zhp");

const threads = @import("../threads.zig");


pub const HandlerError = error{InvalidRequest};

pub const HandlerResponse = struct {
    message: ?[]const u8 = null,
};

pub const Handler = struct {

    pub fn get(self: *Handler, request: *web.Request, response: *web.Response) !void {
        print("Got a file list request", .{});
    }
};
