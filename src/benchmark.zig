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

pub const log_level = .debug;
var gpa = std.heap.GeneralPurposeAllocator(.{}){};

pub fn main() anyerror!void {
    try disc_io_init(2);
}

pub const DiscIO = struct {
    dir: std.fs.Dir,
    file_size: usize = 1024 * 1024,
    file_count: usize = 100,
    test_interations: usize = 10,
};

fn disc_io_init(runs: u8) anyerror!void {
    var tmp = std.testing.tmpDir(.{});
    defer tmp.cleanup();

    var dir_buffer: [std.fs.MAX_PATH_BYTES]u8 = undefined;
    const dir_path = try std.os.getFdPath(tmp.dir.fd, &dir_buffer);
    std.log.info("Diso IO occuring in {s}", .{dir_path});

    const config = DiscIO{ .dir = tmp.dir };

    var test_index: u8 = 0;

    while (test_index < config.test_interations) {
        try disc_io_run(config, test_index);
        test_index += 1;
    }
}

fn disc_io_run(config: DiscIO, test_index: usize) anyerror!void {
    const allocator = &gpa.allocator;

    var file_index = test_index * config.file_count;
    const end_file_index = file_index + config.file_count;

    var file_name_buffer: [std.fs.MAX_PATH_BYTES]u8 = undefined;
    const buffer_slice = file_name_buffer[0..];

    var file_data_buffer = try allocator.alloc(u8, config.file_size);
    defer allocator.free(file_data_buffer);

    const start_timestamp = std.time.milliTimestamp();

    while (file_index < end_file_index) {
        const file_name = try std.fmt.bufPrint(buffer_slice, "file_{d}.ext", .{file_index});

        // Write random data into the file
        std.crypto.random.bytes(file_data_buffer[0..]);
        try config.dir.writeFile(file_name, file_data_buffer);

        file_index += 1;
    }

    try run_command(&[_][]const u8{"sync"});

    const end_timestamp = std.time.milliTimestamp();

    const total_MB = @intToFloat(f64, config.file_size * config.file_count) / 1024.0 / 1024.0;
    const seconds = (@intToFloat(f64, end_timestamp) - @intToFloat(f64, start_timestamp)) / std.time.ms_per_s;

    std.log.info("{d} MB in {d:.2} sec : {d:.2} MB/sec", .{ total_MB, seconds, total_MB / seconds });
}

fn run_command(argv: []const []const u8) anyerror!void {
    const allocator = &gpa.allocator;
    var child = try std.ChildProcess.init(argv, allocator);

    const term = child.spawnAndWait() catch |err| {
        std.log.warn("Unable to spawn {s}: {s}\n", .{ argv[0], @errorName(err) });
        return err;
    };

    switch (term) {
        .Exited => |code| {
            if (code != 0) {
                std.log.warn("Command {s} exited with error code {}", .{ argv, code });
                return error.UncleanExit;
            }
        },
        else => {
            std.log.warn("Command {s} terminated unexpectedly", .{argv});
            return error.UncleanExit;
        },
    }
}
