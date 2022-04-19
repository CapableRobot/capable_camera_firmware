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
const config = @import("../config.zig");

const updateImgCfgStr: []const u8 = 
    \\Updated Imager Parameters:
    \\Width  = {},
    \\Height = {},
    \\FPS    = {}
;

pub const HandlerError = error{InvalidRequest};

pub const HandlerResponse = struct {
    camera: ?config.Camera = null,
    err: ?anyerror = null,
    message: ?[]const u8 = null,
};

pub const ImgCfgHandler = struct {

    pub fn post(self: *ImgCfgHandler, request: *web.Request, response: *web.Response) !void {
        var result = HandlerResponse{ .err = HandlerError.InvalidRequest };
        var content_type = request.headers.getDefault("Content-Type", "");

        if (std.mem.startsWith(u8, content_type, "application/json")) {
            if (!request.read_finished) {
                if (request.stream) |stream| {
                    try request.readBody(stream);
                }
            }

            if (request.content) |content| {
                switch (content.type) {
                    .TempFile => {
                        return error.NotImplemented;
                        // TODO: Parsing should use a stream
                    },
                    .Buffer => {

                        var stream = std.json.TokenStream.init(content.data.buffer);
                        var params = try std.json.parse(config.Camera, &stream, .{.allocator = threads.configuration.allocator});
                        
                        const check = threads.configuration.updateCamera(params);

                        result.camera = params;
                        result.err = check.err;
                        result.message = check.message;
                    },
                }
            }
        }

        try response.headers.append("Content-Type", "application/json");
        try std.json.stringify(result, std.json.StringifyOptions{
            .whitespace = .{ .indent = .{ .Space = 2 } },
        }, response.stream);

    }
};
