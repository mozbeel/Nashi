const std = @import("std");
const c = @cImport({
    @cInclude("SDL3/SDL.h");
    @cInclude("SDL3/SDL_opengl.h");
});
const builtin = @import("builtin");
const gl = @import("gl");

var window : ?*c.SDL_Window = undefined;
pub var running : bool = true;

var gl_context : c.SDL_GLContext = undefined; 
var procs : gl.ProcTable = undefined;

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
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MINOR_VERSION, 3);
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_PROFILE_MASK, c.SDL_GL_CONTEXT_PROFILE_CORE);
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_DOUBLEBUFFER, 1);

    } else {
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MINOR_VERSION, 0);
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_PROFILE_MASK, c.SDL_GL_CONTEXT_PROFILE_ES);
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_DOUBLEBUFFER, 1);

    }

    window =  c.SDL_CreateWindow("Nashi + SDL3", window_width, window_height, c.SDL_WINDOW_OPENGL);

    gl_context = c.SDL_GL_CreateContext(window.?);
    if (gl_context == null) {
        std.debug.print("Failed to create GL context: {s}\n", .{c.SDL_GetError()});
        return error.ErrorIntializingOpenGL;
    }

    if (!c.SDL_GL_MakeCurrent(window, gl_context)) {
        std.log.err("Failed to make GL context current: {s}", .{c.SDL_GetError()});
        return error.ErrorIntializingOpenGL;
    }

    if (!procs.init(c.SDL_GL_GetProcAddress)) return error.ErrorIntializingOpenGL;
    
    gl.makeProcTableCurrent(&procs);
}

pub fn event() void {
    var sdl_event : c.SDL_Event = undefined;
    while (c.SDL_PollEvent(&sdl_event)) {
        switch (sdl_event.type) {
            c.SDL_EVENT_QUIT, c.SDL_EVENT_WINDOW_CLOSE_REQUESTED => running = false,
            else => continue,
        }
    }

}

pub fn iterate() void {
    gl.ClearColor(1, 1, 1, 1);
    gl.Clear(gl.COLOR_BUFFER_BIT);
    _ = c.SDL_GL_SwapWindow(window);
}

pub fn destroy() void {
    _ = c.SDL_GL_DestroyContext(gl_context);
    gl.makeProcTableCurrent(null);

    c.SDL_DestroyWindow(window.?);
    c.SDL_Quit();

}
