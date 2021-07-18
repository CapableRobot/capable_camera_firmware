const std = @import("std");
const print = @import("std").debug.print;
const fs = std.fs;
const mem = @import("std").mem;

const web = @import("zhp");
const handlers = @import("handlers.zig");

var gpa = std.heap.GeneralPurposeAllocator(.{}){};

pub const io_mode = .evented;

const led_driver = @import("led_driver.zig");
const info = @import("info.zig");
const spi = @import("spi.zig");
const gnss = @import("gnss.zig");

fn write_info_json() !void {
    if (try info.stat()) |stat| {
        print("stat {any}\n", .{stat});

        const file = try std.fs.cwd().createFile("test.json", .{});
        defer file.close();
        try std.json.stringify(stat, std.json.StringifyOptions{
            .whitespace = .{ .indent = .{ .Space = 2 } },
        }, file.writer());
    }
}

pub const routes = [_]web.Route{
    web.Route.create("root", "/", handlers.MainHandler),
    web.Route.create("api", "/api", handlers.MainHandler),
    web.Route.create("api info", "/api/info", handlers.InfoHandler),
    web.Route.static("static", "/static/", "static/"),
};

pub fn main() anyerror!void {
    try write_info_json();

    var i2c_fd = try fs.openFileAbsolute("/dev/i2c-1", fs.File.OpenFlags{ .read = true, .write = true });
    defer i2c_fd.close();

    var spi01_fd = try fs.openFileAbsolute("/dev/spidev0.1", fs.File.OpenFlags{ .read = true, .write = true });
    defer spi01_fd.close();

    const led = led_driver.LP50xx{ .fd = i2c_fd };

    if (led.read_register(0x00, 1)) |value| {
        print("CONFIG0 = 0x{s}\n", .{std.fmt.fmtSliceHexUpper(value)});
    }

    if (led.read_register(0x01, 1)) |value| {
        print("CONFIG1 = 0x{s}\n", .{std.fmt.fmtSliceHexUpper(value)});
    }

    led.off();
    led.enable();
    led.set_brightness(0x30);

    led.set(0, [_]u8{ 0, 0, 0 });
    // led.spin();

    var handle = spi.SPI{ .fd = spi01_fd };
    print("SPI configure {any}\n", .{handle.configure(0, 5500)});

    var pos = gnss.init(handle);
    pos.configure();

    while (true) {
        pos.set_next_timeout(1000);

        if (pos.get_pvt()) {
            led.set(0, [_]u8{ 0, 255, 0 });
        } else {
            led.set(0, [_]u8{ 255, 0, 0 });
        }

        std.time.sleep(500 * std.time.ns_per_ms);
    }

    defer std.debug.assert(!gpa.deinit());
    const allocator = &gpa.allocator;

    var app = web.Application.init(allocator, .{ .debug = true });

    defer app.deinit();
    try app.listen("0.0.0.0", 5000);
    try app.start();
}
