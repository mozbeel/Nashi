const std = @import("std");
const builtin = @import("builtin");

const entry = @import("entry.zig");
const c = @import("c.zig").c;
const vk = @import("vulkan");
const za = @import("zalgebra");

pub const renderer = struct {
    window: ?*c.SDL_Window = undefined,
    vk_instance: vk.InstanceProxy = undefined,

    vkb: vk.BaseWrapper = undefined,

    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    
    pub fn init(window: ?*c.SDL_Window) !renderer {
        var self: renderer = .{
            .window = window,
        };
        
        try self.prepare_vulkan();

        return self;
    }

    fn prepare_vulkan(self: *renderer) !void {
        // Load Vulkan
        const loader = c.SDL_Vulkan_GetVkGetInstanceProcAddr();
        if (loader == null) return error.FailedToLoadVulkan;
        
        const vk_loader: vk.PfnGetInstanceProcAddr = @ptrCast(loader);
        self.vkb = vk.BaseWrapper.load(vk_loader);

        try self.create_instance();
    }

    fn create_instance(self: *renderer) !void {
        const allocator = std.heap.c_allocator;

        const app_info: vk.ApplicationInfo = .{
            .p_application_name = "Nashi",
            .application_version = @bitCast(vk.makeApiVersion(1, 0, 0, 0)),
            .p_engine_name = "Nashi",
            .engine_version = @bitCast(vk.makeApiVersion(0, 0, 1, 0)),
            .api_version = @bitCast(vk.API_VERSION_1_2),
        };    

        var extension_names = std.ArrayList([*:0]const u8).init(allocator);
        defer extension_names.deinit();

        try extension_names.append("VK_KHR_portability_enumeration");

        var sdl_exts_count: u32 = 0;
        const sdl_exts = c.SDL_Vulkan_GetInstanceExtensions(&sdl_exts_count);

        try extension_names.appendSlice(@ptrCast(sdl_exts[0..sdl_exts_count]));

        for (extension_names.items) |e| {
            std.log.info("Extension: {s}", .{ e });
        }

    
        const create_info: vk.InstanceCreateInfo = .{
            .p_application_info = &app_info,
            .enabled_extension_count = @intCast(extension_names.items.len),
            .pp_enabled_extension_names = extension_names.items.ptr,
            .enabled_layer_count = 0,
            .flags = .{ .enumerate_portability_bit_khr = true },
        }; 
    
        const instance = try self.vkb.createInstance(&create_info, null);

        const vki = try allocator.create(vk.InstanceWrapper);
        errdefer allocator.destroy(vki);
        vki.* = vk.InstanceWrapper.load(instance, self.vkb.dispatch.vkGetInstanceProcAddr.?);
        self.vk_instance = vk.InstanceProxy.init(instance, vki);
        errdefer self.vk_instance.destroyInstance(null);
    }


    pub fn draw(self: *renderer) void {
        _ = self;
    }

    pub fn resized_window(self: *renderer) void {
        _ = self;
    }

    pub fn destroy(self: *renderer) void {
        self.vk_instance.destroyInstance(null);
    }
};
