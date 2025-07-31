const builtin = @import("builtin");
const entry = @import("entry.zig");
const std = @import("std");

pub fn main() !void {
    try entry.init();
    defer entry.destroy();

    while (entry.running) {
        try entry.event();
        try entry.iterate();
    }
}

