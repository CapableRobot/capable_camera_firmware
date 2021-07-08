const std = @import("std");
const print = @import("std").debug.print;
const fs = std.fs;
const mem = @import("std").mem;

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

const i2c_device = "/dev/i2c-1";

// TODO : generate colorwheel pattern via comptime calls
const colors = [_][3]u8{
    [_]u8{ 255, 0, 0 },
    [_]u8{ 183, 72, 0 },
    [_]u8{ 111, 144, 0 },
    [_]u8{ 39, 216, 0 },
    [_]u8{ 0, 222, 33 },
    [_]u8{ 0, 150, 105 },
    [_]u8{ 0, 78, 177 },
    [_]u8{ 0, 6, 249 },
    [_]u8{ 66, 0, 189 },
    [_]u8{ 138, 0, 117 },
    [_]u8{ 210, 0, 45 },
    [_]u8{ 231, 24, 0 },
    [_]u8{ 159, 96, 0 },
    [_]u8{ 87, 168, 0 },
    [_]u8{ 15, 240, 0 },
    [_]u8{ 0, 198, 57 },
    [_]u8{ 0, 126, 129 },
    [_]u8{ 0, 54, 201 },
    [_]u8{ 18, 0, 237 },
    [_]u8{ 90, 0, 165 },
    [_]u8{ 162, 0, 93 },
    [_]u8{ 234, 0, 21 },
};

fn led_colorwheel(fd: fs.File) !void {
    var step: u8 = 0;
    while (true) {
        for ([_]u8{ 0, 1, 2 }) |index| {
            try led_set(fd, index, colors[(step -% index) % colors.len]);
        }

        if (step < 255) {
            step += 1;
        } else {
            step = 0;
        }

        std.time.sleep(100 * std.time.ns_per_ms);
    }
}

// TODO : fix lifetime issue on result without introducing a global
var command = [_]u8{0x0};
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
    // print("CMD    {any}\n", .{command});
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

fn i2c_read_block(fd: fs.File, addr: u8, register: u8, length: u8) ?[]u8 {
    return i2c_transfer(fd, addr, &[_]u8{register}, 1, length);
}

fn i2c_write_block(fd: fs.File, addr: u8, buffer: []u8) u8 {
    if (i2c_transfer(fd, addr, buffer, @truncate(u8, buffer.len), 0)) |value| {
        // print("i2c_write {} {} {any}\n", .{ addr, value[0], buffer });
        return value[0];
    }
    return 0;
}

const LED = struct {
    addr: u8 = 0x14,
    fd: fs.File,

    pub fn enable(self: LED) void {
        var buffer = [_]u8{ 0x00, 0x40 };
        _ = i2c_write_block(self.fd, self.addr, &buffer);
    }

    pub fn set_brightness_index(self: LED, index: u8, value: u8) void {
        var buffer = [_]u8{ 0x07 + index, value };
        _ = i2c_write_block(self.fd, self.addr, &buffer);
    }

    pub fn set_brightness(self: LED, value: u8) void {
        self.set_brightness_index(0, value);
        self.set_brightness_index(1, value);
        self.set_brightness_index(2, value);
        self.set_brightness_index(3, value);
    }

    pub fn set(self: LED, index: u8, color: [3]u8) void {
        var buffer = [_]u8{ 0x0B + index * 3, color[2], color[1], color[0] };
        _ = i2c_write_block(self.fd, self.addr, &buffer);
    }

    pub fn off(self: LED) void {
        self.set(0, [_]u8{ 0, 0, 0 });
        self.set(1, [_]u8{ 0, 0, 0 });
        self.set(2, [_]u8{ 0, 0, 0 });
        self.set(3, [_]u8{ 0, 0, 0 });
    }

    pub fn read_register(self: LED, register: u8, length: u8) ?[]u8 {
        return i2c_read_block(self.fd, self.addr, register, length);
    }

    pub fn spin(self: LED) void {
        var step: u8 = 0;
        while (true) {
            for ([_]u8{ 0, 1, 2 }) |index| {
                self.set(index, colors[(step -% index) % colors.len]);
            }

            step +%= 1;
            std.time.sleep(100 * std.time.ns_per_ms);
        }
    }
};

pub fn main() anyerror!void {
    var fd = try fs.openFileAbsolute(i2c_device, fs.File.OpenFlags{ .read = true, .write = true });
    defer fd.close();

    const led = LED{ .fd = fd };

    if (led.read_register(0x00, 1)) |value| {
        print("CONFIG0 = 0x{s}\n", .{std.fmt.fmtSliceHexUpper(value)});
    }

    if (led.read_register(0x01, 1)) |value| {
        print("CONFIG1 = 0x{s}\n", .{std.fmt.fmtSliceHexUpper(value)});
    }

    led.off();
    led.enable();
    led.set_brightness(0x30);
    led.spin();
}
