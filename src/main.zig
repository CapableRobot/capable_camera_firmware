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

const web = @import("zhp");
const handlers = @import("handlers.zig");

var gpa = std.heap.GeneralPurposeAllocator(.{}){};
pub const io_mode = .evented;

const spi = @import("bus/spi.zig");

const led_driver = @import("led_driver.zig");
const gnss = @import("gnss.zig");
const threads = @import("threads.zig");
const info = @import("info.zig");
const config = @import("config.zig");

fn write_info_json() !void {
    if (try info.stat()) |stat| {
        print("stat {any}\n", .{stat});

        const file = try std.fs.cwd().createFile("test.json", .{});
        defer file.close();
        try std.json.stringify(stat, std.json.StringifyOptions{
            .whitespace = .{ .indent = .{ .Space = 2 } },
        }, file.writer());
    }
}

pub const routes = [_]web.Route{
    web.Route.create("", "/", handlers.MainHandler),
    web.Route.create("api", "/api", handlers.MainHandler),
    web.Route.create("api/info", "/api/info", handlers.InfoHandler),
    web.Route.create("api/gnss/pvt", "/api/gnss/pvt", handlers.GnssPvtHandler),
    web.Route.static("static", "/static/", "static/"),
};

pub fn main() anyerror!void {
    defer std.debug.assert(!gpa.deinit());
    const allocator = &gpa.allocator;

    var cfg = config.load(allocator);

    var loop: std.event.Loop = undefined;
    try loop.initMultiThreaded();
    defer loop.deinit();

    var i2c_fd = try fs.openFileAbsolute("/dev/i2c-1", fs.File.OpenFlags{ .read = true, .write = true });
    defer i2c_fd.close();

    var spi01_fd = try fs.openFileAbsolute("/dev/spidev0.1", fs.File.OpenFlags{ .read = true, .write = true });
    defer spi01_fd.close();

    const led = led_driver.LP50xx{ .fd = i2c_fd };

    if (led.read_register(0x00, 1)) |value| {
        print("CONFIG0 = 0x{s}\n", .{std.fmt.fmtSliceHexUpper(value)});
    }

    if (led.read_register(0x01, 1)) |value| {
        print("CONFIG1 = 0x{s}\n", .{std.fmt.fmtSliceHexUpper(value)});
    }

    led.off();
    led.enable();
    led.set_brightness(0x30);

    led.set(0, [_]u8{ 0, 0, 0 });
    led.set(1, [_]u8{ 0, 0, 0 });
    led.set(2, [_]u8{ 0, 0, 0 });

    var led_ctx = threads.HeartBeatContext{ .led = led, .idx = 2 };
    try loop.runDetached(allocator, threads.heartbeat_thread, .{led_ctx});

    var handle = spi.SPI{ .fd = spi01_fd };
    print("SPI configure {any}\n", .{handle.configure(0, 5500)});

    var pos = gnss.init(handle);
    pos.configure();

    threads.gnss_ctx = threads.GnssContext{ .led = led, .gnss = &pos };
    try loop.runDetached(allocator, threads.gnss_thread, .{threads.gnss_ctx});

    var recording_ctx = threads.RecordingContext{ .config = cfg.recording, .allocator = allocator };
    try loop.runDetached(allocator, threads.recording_cleanup_thread, .{recording_ctx});

    var app = web.Application.init(allocator, .{ .debug = true });
    var app_ctx = threads.AppContext{ .app = &app, .config = cfg.api };
    try loop.runDetached(allocator, threads.app_thread, .{app_ctx});

    loop.run();
}
