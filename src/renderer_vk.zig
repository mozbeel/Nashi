const std = @import("std");
const builtin = @import("builtin");

const entry = @import("entry.zig");
const c = @import("c.zig").c;
const vk = @import("vulkan");
const za = @import("zalgebra");


pub const renderer = struct {
    window: ?*c.SDL_Window = undefined,
    
    var vkb: vk.BaseWrapper = undefined;

    pub fn init(window: ?*c.SDL_Window) !renderer {
        var self: renderer = .{
            .window = window,
        };
        
        self.prepare_vulkan();

        return self;
    }

    fn prepare_vulkan(self: *renderer) void {
        _ = self;

        // Load Vulkan
        vkb = vk.BaseWrapper.load(@as(vk.PfnGetInstanceProcAddr, @ptrCast(&c.SDL_Vulkan_GetVkGetInstanceProcAddr)));
    }

    pub fn draw(self: *renderer) void {
        _ = self;
    }

    pub fn resized_window(self: *renderer) void {
        _ = self;
    }

    pub fn destroy(self: *renderer) void {
        _ = self;
    }
};
