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

const web = @import("zhp");

const info = @import("info.zig");
const threads = @import("threads.zig");
const recording = @import("recording.zig");

pub const routes = [_]web.Route{
    web.Route.create("", "/", MainHandler),
    web.Route.create("api", "/api", MainHandler),
    web.Route.create("api", "/api/", MainHandler),
    web.Route.create("api/info", "/api/info", InfoHandler),
    web.Route.create("api/gnss/pvt", "/api/gnss/pvt", GnssPvtHandler),
    web.Route.create("api/recordings", "/api/recordings", RecordingIndexHandler),
    web.Route.static("static", "/static/", "static/"),
};

pub const MainHandler = struct {
    pub fn get(self: *MainHandler, request: *web.Request, response: *web.Response) !void {
        try response.headers.append("Content-Type", "text/plain");
        try response.stream.writeAll("");
    }
};

pub const InfoHandler = struct {
    pub fn get(self: *InfoHandler, request: *web.Request, response: *web.Response) !void {
        try response.headers.append("Content-Type", "application/json");

        if (try info.stat()) |stat| {
            try std.json.stringify(stat, std.json.StringifyOptions{
                .whitespace = .{ .indent = .{ .Space = 2 } },
            }, response.stream);
        }
    }
};

pub const GnssPvtHandler = struct {
    pub fn get(self: *GnssPvtHandler, request: *web.Request, response: *web.Response) !void {
        try response.headers.append("Content-Type", "application/json");

        if (threads.gnss_ctx.gnss.last_nav_pvt()) |pvt| {
            try std.json.stringify(pvt, std.json.StringifyOptions{
                .whitespace = .{ .indent = .{ .Space = 2 } },
            }, response.stream);
        }
    }
};

const FolderListing = struct {
    count: usize,
    bytes: u64,
    files: []FileData,
};

const FileData = struct {
    name: []u8,
    mtime: i128,
    ctime: i128,
    size: u64,
};

pub const RecordingIndexHandler = struct {
    pub fn get(self: *RecordingIndexHandler, request: *web.Request, response: *web.Response) !void {
        try response.headers.append("Content-Type", "application/json");

        const ctx = threads.rec_ctx;

        if (recording.directory_listing(ctx.allocator, ctx.config.dir)) |listing| {
            defer ctx.allocator.free(listing.items);

            var list = std.ArrayList(FileData).init(ctx.allocator);

            for (listing.items) |elem| {
                var buffer = ctx.allocator.alloc(u8, elem.name_length) catch {
                    std.log.err("failed to allocate memory for entry: {s}", .{elem.name});

                    response.status = web.responses.INTERNAL_SERVER_ERROR;
                    try response.stream.print("ERROR: failed to allocate memory for entry: {s}", .{elem.name});
                    return;
                };

                var obj = FileData{
                    .name = buffer,
                    .size = elem.size,
                    .mtime = elem.mtime,
                    .ctime = elem.ctime,
                };

                std.mem.copy(u8, obj.name, elem.name[0..elem.name_length]);

                list.append(obj) catch {
                    std.log.err("failed to append entry: {s}", .{elem.name});

                    response.status = web.responses.INTERNAL_SERVER_ERROR;
                    try response.stream.print("ERROR: failed to append entry: {s}", .{elem.name});
                    return;
                };
            }

            const out = FolderListing{
                .count = listing.count,
                .bytes = listing.bytes,
                .files = list.items,
            };

            try std.json.stringify(out, std.json.StringifyOptions{
                .whitespace = .{ .indent = .{ .Space = 2 } },
            }, response.stream);

            defer {
                for (list.items) |node| {
                    ctx.allocator.free(node.name);
                }
                list.deinit();
            }
        }
    }
};
