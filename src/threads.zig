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

const led_driver = @import("led_driver.zig");
const gnss = @import("gnss.zig");
const config = @import("config.zig");
const recording = @import("recording.zig");

const web = @import("zhp");

pub const GnssContext = struct {
    gnss: *gnss.GNSS,
    led: led_driver.LP50xx,
    timeout: u16 = 1000,
};

pub const AppContext = struct {
    config: config.Api,
    app: *web.Application,
};

pub var gnss_ctx: GnssContext = undefined;

pub const HeartBeatContext = struct {
    idx: u8 = 0,
    on: u32 = 100,
    off: u32 = 900,
    color: [3]u8 = [_]u8{ 255, 255, 255 },
    led: led_driver.LP50xx,
};

pub var rec_ctx: RecordingContext = undefined;

pub const RecordingContext = struct {
    config: config.Recording,
    allocator: *std.mem.Allocator,
};

pub fn recording_cleanup_thread(ctx: RecordingContext) void {
    const sleep_ns = @intCast(u64, ctx.config.cleanup_frequency) * std.time.ns_per_s;

    const path = ctx.config.dir;

    if (std.fs.openDirAbsolute(path, .{ .iterate = true, .no_follow = false })) |dir| {} else |err| switch (err) {
        error.FileNotFound => {
            std.log.info("recording directory {s} does not exists, creating folder\n", .{path});
            if (std.fs.makeDirAbsolute(path)) {} else |mkerr| {
                std.log.warn("[{any}] when creating recording directory {s}\n", .{ mkerr, path });
            }
        },
        else => {
            std.log.warn("[{any}] when testin recording directory {s}\n", .{ err, path });
        },
    }

    while (true) {
        const start_ms = std.time.milliTimestamp();

        recording.directory_cleanup(ctx);

        const ellapsed_ns = (std.time.milliTimestamp() - start_ms) * std.time.ns_per_ms;
        std.time.sleep(sleep_ns - @intCast(u64, ellapsed_ns));
    }
}

pub fn heartbeat_thread(ctx: HeartBeatContext) void {
    while (true) {
        ctx.led.set(ctx.idx, ctx.color);
        std.time.sleep(ctx.on * std.time.ns_per_ms);

        ctx.led.set(ctx.idx, [_]u8{ 0, 0, 0 });
        std.time.sleep(ctx.off * std.time.ns_per_ms);
    }
}

pub fn gnss_thread(ctx: GnssContext) void {
    while (true) {
        ctx.gnss.set_next_timeout(ctx.timeout);

        if (ctx.gnss.get_pvt()) {
            ctx.led.set(0, [_]u8{ 0, 255, 0 });

            if (ctx.gnss.last_nav_pvt()) |pvt| {
                print("PVT {s} at ({d:.6},{d:.6}) height {d:.2}", .{ pvt.timestamp, pvt.latitude, pvt.longitude, pvt.height });
                print(" heading {d:.2} velocity ({d:.2},{d:.2},{d:.2}) speed {d:.2}", .{ pvt.heading, pvt.velocity[0], pvt.velocity[1], pvt.velocity[2], pvt.speed });
                print(" fix {d} sat {} flags {} {} {}\n", .{ pvt.fix_type, pvt.satellite_count, pvt.flags[0], pvt.flags[1], pvt.flags[2] });
            }
        } else {
            ctx.led.set(0, [_]u8{ 255, 0, 0 });
        }

        std.time.sleep(std.time.ns_per_ms * @intCast(u64, ctx.timeout / 2));
    }
}

pub fn app_thread(ctx: AppContext) void {
    defer ctx.app.deinit();

    ctx.app.listen("0.0.0.0", ctx.config.port) catch |err| {
        print("app : could not open server port {d}\n", .{ctx.config.port});

        ctx.app.listen("0.0.0.0", 5000) catch |err_fallback| {
            print("app : could not open fallback server port\n", .{});
            return;
        };
    };

    ctx.app.start() catch |err| {
        print("app : could not start\n", .{});
        return;
    };
}
