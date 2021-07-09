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

    const i2c_device = "/dev/i2c-1";
    var fd = try fs.openFileAbsolute(i2c_device, fs.File.OpenFlags{ .read = true, .write = true });
    defer fd.close();

    const led = led_driver.LP50xx{ .fd = fd };

    if (led.read_register(0x00, 1)) |value| {
        print("CONFIG0 = 0x{s}\n", .{std.fmt.fmtSliceHexUpper(value)});
    }

    if (led.read_register(0x01, 1)) |value| {
        print("CONFIG1 = 0x{s}\n", .{std.fmt.fmtSliceHexUpper(value)});
    }

    led.off();
    led.enable();
    led.set_brightness(0x30);

    led.set(0, [_]u8{0,255,0});
    // led.spin();

    defer std.debug.assert(!gpa.deinit());
    const allocator = &gpa.allocator;

    var app = web.Application.init(allocator, .{ .debug = true });

    defer app.deinit();
    try app.listen("0.0.0.0", 5000);
    try app.start();
}
