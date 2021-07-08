const std = @import("std");
const print = std.debug.print;

pub fn uptime() ?[2]f32 {
    const path = "/proc/uptime";
    var buf: [32]u8 = undefined;

    if (std.fs.cwd().readFile(path, &buf)) |bytes| {
        var it = std.mem.split(bytes, " ");

        const uptime_s = it.next() orelse @panic("malformed /proc/uptime");
        const idletime_s = it.next() orelse @panic("malformed /proc/uptime");

        // print("uptime {s} {s}\n", .{ uptime_s, idletime_s[0 .. idletime_s.len - 1] });

        const uptime_f = std.fmt.parseFloat(f32, uptime_s) catch unreachable;
        const idletime_f = std.fmt.parseFloat(f32, idletime_s[0 .. idletime_s.len - 1]) catch unreachable;

        // print("uptime {d} {d}\n", .{ uptime_f, idletime_f });

        return [_]f32{ uptime_f, idletime_f };
    } else |_| {
        return null;
    }

    return null;
}
