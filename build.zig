const std = @import("std");
const Target = std.Target;
const CrossTarget = std.zig.CrossTarget;

pub fn build(b: *std.build.Builder) void {

    // Standard release options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall.
    const mode = b.standardReleaseOptions();

    const exe = b.addExecutable("capable_camera_firmware", "src/main.zig");
    exe.linkSystemLibrary("c");

    exe.setTarget(CrossTarget{
        .cpu_arch = Target.Cpu.Arch.arm,
        .os_tag = Target.Os.Tag.linux,
        .abi = Target.Abi.musleabihf,
    });

    exe.setBuildMode(mode);
    exe.install();

    const run_cmd = exe.run();
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
