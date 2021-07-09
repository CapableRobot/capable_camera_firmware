const std = @import("std");
const fs = std.fs;
const mem = std.mem;
const print = std.debug.print;

const proce_stat_core_times = packed struct {
    user: u64 = 0,
    nice: u64 = 0,
    system: u64 = 0,
    idle: u64 = 0,
    iowait: u64 = 0,
    irq: u64 = 0,
    softirq: u64 = 0,
    steal: u64 = 0,
    guest: u64 = 0,
    guestnice: u64 = 0,
};

// TODO : don't hard code this
const num_cores = 4;

const proc_stat = struct {
    cpu: proce_stat_core_times,
    core: [num_cores]proce_stat_core_times,
};

fn parse_cpu_line(line: []const u8) proce_stat_core_times {
    var it = std.mem.split(line, " ");

    const ident = it.next() orelse @panic("stat: no core name");

    // 'cpu' has two spaces after it instead of one, so advance over the null between them
    if (ident.len == 3) {
        _ = it.next();
    }

    return proce_stat_core_times{
        .user = std.fmt.parseInt(u64, it.next() orelse @panic("stat: no core user time"), 10) catch unreachable,
        .nice = std.fmt.parseInt(u64, it.next() orelse @panic("stat: no core nice time"), 10) catch unreachable,
        .system = std.fmt.parseInt(u64, it.next() orelse @panic("stat: no core system time"), 10) catch unreachable,
        .idle = std.fmt.parseInt(u64, it.next() orelse @panic("stat: no core idle time"), 10) catch unreachable,
        .iowait = std.fmt.parseInt(u64, it.next() orelse @panic("stat: no core iowait time"), 10) catch unreachable,
        .irq = std.fmt.parseInt(u64, it.next() orelse @panic("stat: no core irq time"), 10) catch unreachable,
        .softirq = std.fmt.parseInt(u64, it.next() orelse @panic("stat: no core softirq time"), 10) catch unreachable,
        .steal = std.fmt.parseInt(u64, it.next() orelse @panic("stat: no core steal time"), 10) catch unreachable,
        .guest = std.fmt.parseInt(u64, it.next() orelse @panic("stat: no core guest time"), 10) catch unreachable,
        .guestnice = std.fmt.parseInt(u64, it.next() orelse @panic("stat: no core guestnice time"), 10) catch unreachable,
    };
}

pub fn stat() !?proc_stat {
    const path = "/proc/stat";
    var buf: [1024]u8 = undefined;

    var fd = fs.openFileAbsolute(path, fs.File.OpenFlags{ .read = true, .write = false }) catch unreachable;
    defer fd.close();

    const reader = std.io.bufferedReader(fd.reader()).reader();

    // Fill struct with default (e.g. 0) core_time structs
    var data: proc_stat = undefined;
    data.cpu = proce_stat_core_times{};

    var core_idx: usize = 0;
    while (core_idx < num_cores) {
        data.core[core_idx] = proce_stat_core_times{};
        core_idx += 1;
    }

    core_idx = 0;

    while (try reader.readUntilDelimiterOrEof(&buf, '\n')) |line| {
        var it = std.mem.split(line, " ");
        const ident = it.next() orelse @panic("malformed /proc/stat");
        print("{s}\n", .{line});

        if (ident.len >= 3 and mem.eql(u8, ident[0..3], "cpu")) {
            const times = parse_cpu_line(line);

            if (ident.len == 3) {
                data.cpu = times;
            } else {
                data.core[core_idx] = times;
                core_idx += 1;
            }
        }
    }

    return data;
}

pub fn uptime() ?[2]f32 {
    const path = "/proc/uptime";
    var buf: [32]u8 = undefined;

    if (fs.cwd().readFile(path, &buf)) |bytes| {
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
