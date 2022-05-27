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
const mem = std.mem;

pub var init_uptime: f32 = 0.0;
pub var init_uptime_ms: i64 = 0.0;
pub var init_timestamp: i64 = 0.0;

// Returns seconds since system was turned on.
// Requires that init_uptime and init_timestamp be set at program boot.
// This allows calls to be offset from initially stored uptime via system time offsets
// which is faster than reading /proc/uptime for every long line
pub fn logstamp() f32 {
    const millis = std.time.milliTimestamp() - init_timestamp;
    return init_uptime + @intToFloat(f32, millis) / 1000.0;
}

pub fn timestamp() i64 {
    const millis = std.time.milliTimestamp() - init_timestamp;
    return init_uptime_ms + millis;
}

pub fn uptime_idletime() ?[2]f32 {
    const path = "/proc/uptime";
    var buf: [32]u8 = undefined;

    if (fs.cwd().readFile(path, &buf)) |bytes| {
        var it = std.mem.split(bytes, " ");

        const uptime_s = it.next() orelse @panic("malformed /proc/uptime");
        const idletime_s = it.next() orelse @panic("malformed /proc/uptime");

        const uptime_f = std.fmt.parseFloat(f32, uptime_s) catch unreachable;
        const idletime_f = std.fmt.parseFloat(f32, idletime_s[0 .. idletime_s.len - 1]) catch unreachable;

        return [_]f32{ uptime_f, idletime_f };
    } else |_| {
        return null;
    }

    return null;
}

pub fn uptime() f32 {
    if (uptime_idletime()) |value| {
        return value[0];
    }

    return 0;
}

pub fn init() void {
    init_uptime = uptime();
    init_uptime_ms = @floatToInt(i64, init_uptime * 1000);

    init_timestamp = std.time.milliTimestamp();
}
