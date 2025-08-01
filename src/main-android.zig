const builtin = @import("builtin");
const entry = @import("entry.zig");
const android = @import("android");
const std = @import("std");

pub const std_options: std.Options = if (builtin.abi.isAndroid())
    .{
        .logFn = android.logFn,
    }
else
    .{};

export fn SDL_main(argc: c_int, argv: [*]*?*const u8) callconv(.c) c_int {
    _ = argc;
    _ = argv;
    std.log.debug("Hello Nashi!", .{});
    entry.init() catch return 1;
    defer entry.destroy();

    while (entry.running) {
        entry.event() catch return 1;
        entry.iterate() catch return 1;
    }
    return 0;
}



