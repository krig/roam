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
    exe.linkFramework("OpenGL");
    exe.addIncludePath(.{ .cwd_relative = "stb/" });
    exe.addIncludePath(.{ .cwd_relative = "src/" });
    exe.addCSourceFiles(.{
        .root = b.path("src"),
        .files = &.{
            "roam_linux.c",
        },
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

    // TODO: handle linux vs. mac
    // TODO: handle stb code quality
    // TODO: handle STL2
    // TODO: handle GLEW

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);

    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the game");
    run_step.dependOn(&run_cmd.step);
}
