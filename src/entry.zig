const std = @import("std");
const c = @import("c.zig").c;
const builtin = @import("builtin");
const renderer_vk = @import("renderer_vk.zig");

var window : ?*c.SDL_Window = undefined;
pub var running : bool = true;

var renderer: renderer_vk.renderer = undefined;

pub var start_time: i64 = undefined;

pub fn init() !void {
    if (c.SDL_Init(c.SDL_INIT_VIDEO) == false) {
        std.log.err("Failed to initialize SDL", .{});
        return error.InitSDLFailed;
    }
    
    var window_width : u16 = 1280;
    var window_height : u16 = 720;

    if (builtin.abi.isAndroid() or builtin.target.os.tag == .ios) {
        const display_mode = c.SDL_GetCurrentDisplayMode(c.SDL_GetPrimaryDisplay());

        if (display_mode.?) |d| {
            window_width = @intCast(d.*.w);
            window_height = @intCast(d.*.h);
        } else {
            std.log.err("Couldn't get screen size", .{});
            return error.InitSDLWindowSizeFailed;
        }
    }

    if (!builtin.target.abi.isAndroid() and builtin.target.os.tag != .ios and builtin.target.os.tag != .emscripten) {
        //_ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        //_ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MINOR_VERSION, 3);
        //_ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_PROFILE_MASK, c.SDL_GL_CONTEXT_PROFILE_CORE);

    } else {
        //_ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        //_ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MINOR_VERSION, 0);
        //_ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_PROFILE_MASK, c.SDL_GL_CONTEXT_PROFILE_ES);

    }

    window =  c.SDL_CreateWindow("Nashi + SDL3", window_width, window_height, 
          c.SDL_WINDOW_VULKAN 
        | c.SDL_WINDOW_RESIZABLE);

    if (window == null) {
        std.log.err("Failed to create SDL window: {s}", .{c.SDL_GetError()});
        return error.InitSDLWindowFailed;
    }
    start_time = std.time.microTimestamp();

    renderer = try renderer_vk.renderer.init(window);
}

pub fn event() !void {
    var sdl_event : c.SDL_Event = undefined;
    while (c.SDL_PollEvent(&sdl_event)) {
        switch (sdl_event.type) {
            c.SDL_EVENT_QUIT, c.SDL_EVENT_WINDOW_CLOSE_REQUESTED => running = false,
            c.SDL_EVENT_WINDOW_RESIZED => renderer.resized_window(),
            else => continue,
        }
    }

}

pub fn iterate() !void {
    renderer.draw();
}

pub fn destroy() void {
    renderer.destroy();

    c.SDL_DestroyWindow(window.?);
    c.SDL_Quit();

}
