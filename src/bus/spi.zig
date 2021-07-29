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
const print = std.debug.print;

const c = @cImport({
    @cInclude("linux/spi/spidev.h");
    @cInclude("sys/ioctl.h");
    @cInclude("sys/errno.h");
});

// const Config = extern struct {
//     unsigned int mode;
//     unsigned int bits_per_word;
//     unsigned int speed_hz;
//     unsigned int delay_us;
// };

const DEFAULT_MODE = 0;
const DEFAULT_SPEED = 1000000;

const spi_ioc_transfer = extern struct {
    tx_buf: u64,
    rx_buf: u64,
    len: u32,
    speed_hz: u32 = 1000000,
    delay_usecs: u16 = 10,
    bits_per_word: u8 = 8,
    cs_change: u8 = 0,
    tx_nbits: u8 = 0,
    rx_bits: u8 = 0,
    pad: u16 = 0,
};

const SPI_DEFAULT_CHUNK_SIZE = 4096;

// TODO : build this magic number based on the C macros, instead of hard coding it.
const IOC_MESSAGE = 0x40206b00;

pub const SPI = struct {
    fd: fs.File,
    mode: u8 = DEFAULT_MODE,
    speed_hz: u32 = DEFAULT_SPEED,
    // config: Config,
    chunk_size: u32 = SPI_DEFAULT_CHUNK_SIZE,

    pub fn configure(self: *SPI, mode: u8, speed: u32) c_int {
        var rv = c.ioctl(self.fd.handle, c.SPI_IOC_WR_MODE, &mode);
        if (rv == -1) {
            print("spi:configure : cannot set mode to {}\n", .{mode});
            return rv;
        }

        self.mode = mode;

        rv = c.ioctl(self.fd.handle, c.SPI_IOC_WR_MAX_SPEED_HZ, &speed);
        if (rv == -1) {
            print("spi:configure : cannot set speed to {}\n", .{speed});
        }

        self.speed_hz = speed;

        return rv;
    }

    pub fn transfer(self: *SPI, to_write: [*]u8, to_read: [*]u8, len: usize) c_int {
        var tfer = spi_ioc_transfer{
            .tx_buf = @intCast(u64, @ptrToInt(&to_write[0])),
            .rx_buf = @intCast(u64, @ptrToInt(&to_read[0])),
            .len = len,
            .speed_hz = self.speed_hz,
        };

        return c.ioctl(self.fd.handle, IOC_MESSAGE, &tfer);
    }

    pub fn write(self: *SPI, buffer: []u8) u8 {
        if (buffer.len < SPI_DEFAULT_CHUNK_SIZE) {
            var tfer = spi_ioc_transfer{
                .tx_buf = @intCast(u64, @ptrToInt(&buffer[0])),
                .rx_buf = 0,
                .len = buffer.len,
            };

            return @intCast(u8, c.ioctl(self.fd.handle, IOC_MESSAGE, &tfer));
        } else {
            print("SPI : Trying to write a buffer larger than supported chunk size", .{});
            return 0;
        }
    }

    pub fn read_byte(self: *SPI) ?u8 {
        var buffer = [_]u8{0};

        var tfer = spi_ioc_transfer{
            .tx_buf = 0,
            .rx_buf = @intCast(u64, @ptrToInt(&buffer[0])),
            .len = 1,
            .speed_hz = self.speed_hz,
        };

        if (c.ioctl(self.fd.handle, IOC_MESSAGE, &tfer) != -1) {
            return buffer[0];
        }

        return null;
    }

    pub fn read_into(self: *SPI, to_read: [*]u8, len: u8) c_int {
        if (length < 256) {
            var buffer = [_]u8{0} ** 256;

            var tfer = spi_ioc_transfer{
                .tx_buf = 0,
                .rx_buf = @intCast(u64, @ptrToInt(&to_read[0])),
                .len = len,
                .speed_hz = self.speed_hz,
            };

            return c.ioctl(self.fd.handle, IOC_MESSAGE, &tfer);
        } else {
            print("SPI : Trying to read a buffer larger than supported chunk size", .{});
            return null;
        }
    }
};
