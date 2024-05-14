const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "roam",
        .target = target,
        .optimize = optimize,
    });
    exe.linkLibC();
    exe.linkSystemLibrary("glew");
    exe.linkSystemLibrary("sdl2");
    if (target.result.os.tag == .macos) {
        exe.linkFramework("OpenGL");
    } else {
        exe.linkSystemLibrary("gl");
    }
    exe.addIncludePath(.{ .cwd_relative = "stb/" });
    exe.addIncludePath(.{ .cwd_relative = "src/" });
    const sources: []const []const u8 = if (target.result.os.tag == .windows)
        &[_][]const u8{"roam_windows.c"}
    else
        &[_][]const u8{"roam_linux.c"};
    exe.addCSourceFiles(.{
        .root = b.path("src"),
        .files = sources,
        .flags = &.{
            "-Wall",
            "-Wextra",
            "-pedantic",
            "-Wno-unused-function",
            "-Wno-unused-parameter",
            "-mtune=native",
            "-fno-strict-aliasing",
            "-fno-sanitize=undefined",
            "-std=gnu17",
            "-DNDEBUG",
        },
    });

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);

    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the game");
    run_step.dependOn(&run_cmd.step);
}
