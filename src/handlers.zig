const std = @import("std");
const web = @import("zhp");

const info = @import("info.zig");

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
            try std.json.stringify(
                stat, 
                std.json.StringifyOptions{
                    .whitespace = .{ .indent = .{ .Space = 2 } },
                }, 
                response.stream
            );
        }   
    }
};