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
const exif = @import("exif.zig");

const web = @import("zhp");

pub const GnssContext = struct {
    gnss: *gnss.GNSS,
    led: led_driver.LP50xx,
    rate: u16 = 1000,
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
    server: *std.net.StreamServer,
    stop: std.atomic.Atomic(bool),
    last_frame: usize = 0,
    gnss: GnssContext,
};

pub const CameraContext = struct {
    config: config.Camera,
    socket: []const u8,
    allocator: *std.mem.Allocator,
};

pub var camera_ctx: CameraContext = undefined;

const JPEG_SOI = [_]u8{ 0xFF, 0xD8 };
const JPEG_EOI = [_]u8{ 0xFF, 0xD9 };

const PUB = "PUB ";
const EOL = "\r\n";

fn find_som(buffer: []const u8, start: usize, end: usize) ?usize {
    return std.mem.indexOf(u8, buffer[start..end], PUB[0..]);
}

pub fn recording_server_thread(ctx: *RecordingContext) void {
    while (true) {
        const conn = ctx.server.accept() catch |err| {
            std.log.err("REC | server accept | ERR {}", .{err});
            continue;
        };

        std.log.info("REC | client connected", .{});
        defer conn.stream.close();

        var frame = async handle_connection(ctx, conn);
        await frame;
    }
}

fn handle_connection(ctx: *RecordingContext, conn: std.net.StreamServer.Connection) void {
    // Create large buffer for collecting image data into
    // Should be larger than the largest image size we expect
    var buffer: [1024 * 1024 * 4]u8 = undefined;
    std.mem.set(u8, buffer[0..], 0);

    // Buffer for incoming UDP data (should be larger than max MTU size)
    var incoming: [100000]u8 = undefined;

    // Current position within the buffer to write into
    var head: usize = 0;

    // Current position within the buffer to read from
    var read: usize = 0;

    // Positions within the image buffer where JPEG start / end bytes are
    var idx_start: usize = 0;
    var idx_end: usize = 0;

    // Flags for JPEG SOF / EOF
    var found_start: bool = false;
    var found_end: bool = false;

    // TODO : detect last frame count on disk and start above that number
    var frame_count: usize = 0;

    // Error recovery flag -- when non-null the buffers and flags will be reset
    var reset_to: ?usize = null;

    var topic_name: ?[]const u8 = null;
    var message_size: usize = 0;

    while (true) {
        if (reset_to != null) {
            head = reset_to.?;
            read = 0;

            frame_count += 1;
            found_start = false;
            found_end = false;

            reset_to = null;
        }

        // std.log.info("SOCK READ SIZE {}", .{ctx.sock.getReadBufferSize()});

        const data_len = conn.stream.reader().read(incoming[0..]) catch |err| {
            std.log.err("REC RECV | ERR {}", .{err});
            continue;
        };

        if (data_len == 0) {
            std.log.info("REC RECV | client disconnected", .{});
            break;
        }

        // std.log.info("head {} data_len {}", .{ head, data_len });

        if (head + data_len > buffer.len) {
            std.log.err("REC RECV | buffer will overflow. reset.", .{});
            std.mem.set(u8, incoming[0..], 0);
            reset_to = 0;
            continue;
        }

        std.mem.copy(u8, buffer[head .. head + data_len], incoming[0..data_len]);

        if (!found_start) {

            // Using read as start of search space as PUB might be split across UDP packets
            // and head.. might skip over start of sequence.

            // if (std.mem.indexOf(u8, buffer[read..head+data_len], PUB[0..])) |idx_msg_start| {
            if (find_som(&buffer, read, head + data_len)) |idx_msg_start| {
                const idx_topic_start = idx_msg_start + PUB.len;

                if (std.mem.indexOf(u8, buffer[idx_topic_start .. idx_topic_start + 20], " ")) |idx_space| {
                    topic_name = buffer[idx_topic_start .. idx_topic_start + idx_space];

                    // std.log.info(". idx_msg_start {}", .{idx_msg_start});
                    // std.log.info(". idx_space {}", .{idx_space});
                    // std.log.info(". topic {s}", .{topic_name});

                    if (std.mem.indexOf(u8, buffer[idx_topic_start .. idx_topic_start + 20], "\r\n")) |idx_eol| {
                        if (std.fmt.parseInt(usize, buffer[idx_topic_start + idx_space + 1 .. idx_topic_start + idx_eol], 10)) |value| {
                            message_size = value;
                            // std.log.info(". size {}", .{message_size});

                            // Advance read to end of PUB line (including line break)
                            read += idx_topic_start + idx_eol + 2;

                            if (buffer[read] == JPEG_SOI[0] and buffer[read + 1] == JPEG_SOI[1]) {
                                idx_start = read;
                                // std.log.info(". idx_start {}", .{idx_start});
                                found_start = true;
                            } else {
                                std.log.err("REC RECV | did not find start of JPEG data at {}", .{read});
                                reset_to = 0;
                                continue;
                            }
                        } else |err| {
                            std.log.err("REC RECV | cannot parse message size : {s}", .{buffer[idx_topic_start + idx_space + 1 .. idx_topic_start + idx_eol]});
                            reset_to = 0;
                            continue;
                        }
                    }
                }
            }
        }

        // std.log.info("acculum {} to {}", .{ read, head + data_len });

        if (found_start and !found_end and (head + data_len - read) >= message_size + 2) {
            // std.log.info("buffer start {s}", .{std.fmt.fmtSliceHexUpper(buffer[0 .. read + 20])});
            // std.log.info("buffer end {s}", .{std.fmt.fmtSliceHexUpper(buffer[read + message_size - 4 .. read + message_size + 4])});

            // Check for the EOL bytes and for the valid end of a JPEG frame
            if (buffer[read + message_size - 2] == JPEG_EOI[0] and
                buffer[read + message_size - 1] == JPEG_EOI[1] and
                buffer[read + message_size] == EOL[0] and
                buffer[read + message_size + 1] == EOL[1])
            {
                idx_end = read + message_size;
                // std.log.info("idx_end {}", .{idx_end});
                found_end = true;
            } else {
                if (std.mem.indexOf(u8, buffer[idx_start..], PUB[0..])) |value| {
                    std.log.info("REC RECV | NO EOL. FOUND PUB at {}", .{idx_start + value});

                    const length: usize = head + data_len - (value + idx_start);
                    std.mem.copy(u8, buffer[0..length], buffer[idx_start + value .. head + data_len]);
                    std.mem.set(u8, buffer[length..], 0);

                    reset_to = length;
                    continue;
                } else {
                    std.log.info("REC RECV | NO EOL | HARD RESET", .{});
                    std.mem.set(u8, buffer[0..], 0);

                    reset_to = 0;
                    continue;
                }
            }
        }

        head += data_len;

        if (found_start and found_end) {
            std.log.info("REC RECV | Frame {} is {} from {} to {}", .{ frame_count, idx_end - idx_start, idx_start, idx_end });

            const filename = std.fmt.allocPrint(ctx.allocator, "{s}/frame_{d}.jpg", .{ ctx.config.dir, frame_count }) catch |err| {
                std.log.err("REC RECV | could not create filename", .{});
                reset_to = 0;
                continue;
            };

            defer ctx.allocator.free(filename);

            var file = std.fs.cwd().createFile(filename, .{}) catch |err| {
                std.log.err("REC RECV | could not create file : {}", .{err});
                reset_to = 0;
                continue;
            };

            defer file.close();

            write_image(ctx, file, buffer[idx_start..idx_end]) catch |err| {
                std.log.err("REC RECV | could write image : {}", .{err});
                reset_to = 0;
                continue;
            };

            ctx.last_frame = frame_count;

            // Copy any partial data we have to the start of the acculumation buffer
            if (idx_end + 2 < head) {
                std.log.info("REC RECV | copy tail bytes : {} {}", .{ idx_end, head });
                std.mem.copy(u8, buffer[0 .. head - idx_end - 2], buffer[idx_end + 2 .. head]);
                reset_to = head - idx_end - 2;
            } else {
                reset_to = 0;
            }
        }
    }
}

fn write_image(ctx: *RecordingContext, file: std.fs.File, buffer: []const u8) !void {
    var buf_stream = std.io.bufferedWriter(file.writer());
    const st = buf_stream.writer();

    var exif_tags = exif.init();

    if (ctx.gnss.gnss.last_nav_pvt()) |pvt| {
        exif_tags.set_gnss(pvt);
    }

    if (exif_tags.bytes()) |exif_array| {
        const exif_len = exif_array.len + 2;
        const exif_buffer = exif_array.constSlice();

        // std.log.info("EXIF | [{d}]     {s}", .{ exif_buffer.len, std.fmt.fmtSliceHexUpper(exif_buffer) });

        try st.writeAll(JPEG_SOI[0..]);
        try st.writeAll(exif.APP0_HEADER[0..]);
        try st.writeAll(exif.MARK_APP1[0..]);

        try st.writeByte(@truncate(u8, exif_len >> 8));
        try st.writeByte(@truncate(u8, exif_len));

        try st.writeAll(exif_buffer);

        try st.writeAll(buffer[exif.image_offset..]);
    } else {
        // No EXIF data, so just write out the image part of the buffer -- it is valid JPEG data.
        std.log.info("EXIF embedding failed", .{});
        try st.writeAll(buffer[0..]);
    }

    try buf_stream.flush();
}

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
        // ctx.led.set(ctx.idx, ctx.color);
        std.time.sleep(ctx.on * std.time.ns_per_ms);

        // ctx.led.set(ctx.idx, [_]u8{ 0, 0, 0 });
        std.time.sleep(ctx.off * std.time.ns_per_ms);
    }
}

pub fn gnss_thread(ctx: GnssContext) void {
    ctx.gnss.set_timeout(ctx.rate + 50);

    while (true) {
        // ctx.gnss.set_next_timeout(ctx.rate * 2);

        if (ctx.gnss.get_pvt()) {
            if (ctx.gnss.last_nav_pvt()) |pvt| {
                if (pvt.fix_type == 0) {
                    // If no position fix, color LED orange
                    ctx.led.set(1, [_]u8{ 255, 127, 0 });
                } else {
                    // If there is a fix, color LED green
                    ctx.led.set(1, [_]u8{ 0, 255, 0 });
                }

                print("PVT {s} at ({d:.6},{d:.6}) height {d:.2}", .{ pvt.timestamp, pvt.latitude, pvt.longitude, pvt.height });
                print(" heading {d:.2} velocity ({d:.2},{d:.2},{d:.2}) speed {d:.2}", .{ pvt.heading, pvt.velocity[0], pvt.velocity[1], pvt.velocity[2], pvt.speed });
                print(" fix {d} sat {} flags {} {} {}\n", .{ pvt.fix_type, pvt.satellite_count, pvt.flags[0], pvt.flags[1], pvt.flags[2] });
            }
        } else {
            // If no communications, color LED red
            ctx.led.set(1, [_]u8{ 255, 0, 0 });
        }

        // std.time.sleep(std.time.ns_per_ms * @intCast(u64, ctx.rate / 4));
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
