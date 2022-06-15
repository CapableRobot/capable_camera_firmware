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

pub var state: State = undefined;

// Returns seconds since system was turned on.
// Requires that init_uptime and init_timestamp be set at program boot.
// This allows calls to be offset from initially stored uptime via system time offsets
// which is faster than reading /proc/uptime for every long line
pub fn logstamp() f32 {
    return state.logstamp();
}

pub fn timestamp() i64 {
    return state.timestamp();
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

pub const State = struct {
    _uptime: f32,
    _uptime_ms: i64,
    _timestamp: i64,

    pub fn logstamp(self: *State) f32 {
        const millis = std.time.milliTimestamp() - self._timestamp;
        return self._uptime + @intToFloat(f32, millis) / 1000.0;
    }

    pub fn timestamp(self: *State) i64 {
        const millis = std.time.milliTimestamp() - self._timestamp;
        return self._uptime_ms + millis;
    }
};

pub fn init() void {
    const start = uptime();
    const stamp = std.time.milliTimestamp();

    state = State{
        ._uptime = start,
        ._uptime_ms = @floatToInt(i64, start * 1000),
        ._timestamp = stamp,
    };
}
