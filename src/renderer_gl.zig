const std = @import("std");
const gl = @import("gl");
const c = @import("c.zig").c;

pub const renderer = struct {
    gl_context : c.SDL_GLContext = undefined, 
    procs : gl.ProcTable = undefined,
    window : ?*c.SDL_Window = undefined,

    pub fn init(window: ?*c.SDL_Window) !renderer {
        var self : renderer = .{
            .window = window,
        };
        try self.prepare_opengl();

        return self;
    }


    fn prepare_opengl(self: *renderer) !void {
        const wnd = self.window;
        self.gl_context = c.SDL_GL_CreateContext(wnd);

        if (self.gl_context == null) {
            std.debug.print("Failed to create GL context: {s}\n", .{c.SDL_GetError()});
            return error.ErrorIntializingOpenGL;
        }

        if (!c.SDL_GL_MakeCurrent(wnd, self.gl_context)) {
            std.log.err("Failed to make GL context current: {s}", .{c.SDL_GetError()});
            return error.ErrorIntializingOpenGL;
        }

        if (!self.procs.init(c.SDL_GL_GetProcAddress)) return error.ErrorIntializingOpenGL;

        gl.makeProcTableCurrent(&self.procs);

    }
    
    pub fn draw(self: *renderer) void {
        gl.ClearColor(1, 1, 1, 1);
        gl.Clear(gl.COLOR_BUFFER_BIT);
        _ = c.SDL_GL_SwapWindow(self.window);

    }

    pub fn destroy(self: *renderer) void {
        _ = c.SDL_GL_DestroyContext(self.gl_context);
        gl.makeProcTableCurrent(null);

    }
};
