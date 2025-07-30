const std = @import("std");
const gl = @import("gl");
const c = @import("c.zig").c;

pub const renderer = struct {
    var procs : gl.ProcTable = undefined; // HAS to be defined as var for some reason here
   
    gl_context : c.SDL_GLContext = undefined,
    window : ?*c.SDL_Window = undefined,
    vbo: c_uint = 0,

    const vertices = &.{
        -0.5, -0.5, 0.0,
        0.5, -0.5, 0.0,
        0.0, 0.5, 0.0,
    };

    pub fn init(window: ?*c.SDL_Window) !renderer {
        var self : renderer = .{
            .window = window,
        };
        try self.prepare_opengl();
        
    
        return self;
    }

    fn prepare_opengl(self: *renderer) !void {
        self.gl_context = c.SDL_GL_CreateContext(self.window);

        if (self.gl_context == null) {
            std.debug.print("Failed to create GL context: {s}\n", .{c.SDL_GetError()});
            return error.ErrorIntializingOpenGL;
        }

        if (!c.SDL_GL_MakeCurrent(self.window, self.gl_context)) {
            std.log.err("Failed to make GL context current: {s}", .{c.SDL_GetError()});
            return error.ErrorIntializingOpenGL;
        }

        if (!procs.init(c.SDL_GL_GetProcAddress)) return error.ErrorIntializingOpenGL;

        gl.makeProcTableCurrent(&procs);

    }

    pub fn draw(self: *renderer) void {
        gl.ClearColor(1, 1, 1, 1);
        gl.Clear(gl.COLOR_BUFFER_BIT);
        _ = c.SDL_GL_SwapWindow(self.window.?);

    }

    pub fn destroy(self: *renderer) void {
        _ = c.SDL_GL_DestroyContext(self.gl_context);
        gl.makeProcTableCurrent(null);

    }
};
