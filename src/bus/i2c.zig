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
const print = std.debug.print;
const fs = std.fs;

const c = @cImport({
    @cInclude("linux/i2c.h");
    @cInclude("linux/i2c-dev.h");
    @cInclude("sys/ioctl.h");
    @cInclude("sys/errno.h");
});

const i2c_msg = extern struct {
    addr: u16,
    flags: u16,
    len: u16,
    buf: *u8,
};

const i2c_rdwr_ioctl_data = extern struct {
    msgs: *i2c_msg,
    nmsgs: u32,
};

// TODO : fix lifetime issue on result without introducing a global
var result = [_]u8{0x0} ** c.I2C_SMBUS_BLOCK_MAX;

fn i2c_transfer(fd: fs.File, addr: u8, write_buffer: []u8, write_length: u8, read_length: u8) ?[]u8 {
    if (write_length > c.I2C_SMBUS_BLOCK_MAX) {
        print("i2c_transfer with write length greater than I2C_SMBUS_BLOCK_MAX", .{});
        return null;
    }

    if (read_length > c.I2C_SMBUS_BLOCK_MAX) {
        print("i2c_transfer with read length greater than I2C_SMBUS_BLOCK_MAX", .{});
        return null;
    }

    var messages: [2]i2c_msg = undefined;
    var num_messages: u8 = 1;

    var write_msg = i2c_msg{ .addr = addr, .flags = 0, .len = write_length, .buf = &write_buffer[0] };
    var read_msg = i2c_msg{ .addr = addr, .flags = c.I2C_M_RD, .len = read_length, .buf = &result[0] };

    if (write_length > 0) {
        messages[0] = write_msg;
        if (read_length > 0) {
            messages[1] = read_msg;
            num_messages = 2;
        }
    } else {
        messages[0] = read_msg;
    }

    const request = i2c_rdwr_ioctl_data{ .msgs = &messages[0], .nmsgs = num_messages };
    var rv = c.ioctl(fd.handle, c.I2C_RDWR, &request);

    // print("MSG[0] {}\n", .{messages[0]});
    // print("MSG[1] {}\n", .{messages[1]});
    // print("RET    {any}\n", .{rv});
    // print("CMD    {any}\n", .{write_buffer});
    //
    // if (read_length > 0) {
    //     print("RSLT   {any}\n", .{result[0..read_length]});
    // }

    if (rv > 0) {
        if (read_length == 0) {
            return &[_]u8{write_length};
        } else {
            return result[0..read_length];
        }
    } else {
        return null;
    }
}

pub fn read_block(fd: fs.File, addr: u8, register: u8, length: u8) ?[]u8 {
    return i2c_transfer(fd, addr, &[_]u8{register}, 1, length);
}

pub fn write_block(fd: fs.File, addr: u8, buffer: []u8) u8 {
    if (i2c_transfer(fd, addr, buffer, @truncate(u8, buffer.len), 0)) |value| {
        // print("i2c_write {} {} {any}\n", .{ addr, value[0], buffer });
        return value[0];
    }
    return 0;
}
