const std = @import("std");
const gl = @import("gl");
const c = @import("c.zig").c;

pub const renderer = struct {
    gl_context : c.SDL_GLContext = undefined,
    window : ?*c.SDL_Window = undefined,
    vbo: c_uint = 0,
    vertex_shader: c_uint = 0,

    var procs : gl.ProcTable = undefined; // HAS to be defined as var for some reason here
    const vertices = [_]f32{
        -0.5, -0.5, 0.0,
        0.5, -0.5, 0.0,
        0.0, 0.5, 0.0,
    };
    
    const vertex_shader_source: [*]const [*]const u8 = 
        \\#version 330 core 
        \\layout (location = 0) in vec3 aPos;
        \\
        \\void main() {
        \\  gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
        \\
        \\}
    ;

    pub fn init(window: ?*c.SDL_Window) !renderer {
        var self : renderer = .{
            .window = window,
        };
        try self.prepare_opengl();
       
        self.create_buffers();
        self.create_shaders();
    
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

    fn create_buffers(self: *renderer) void {
        var vbos_to_create = [_]c_uint{ self.vbo };
        gl.GenBuffers(1, vbos_to_create[0..].ptr);
        gl.BindBuffer(gl.ARRAY_BUFFER, self.vbo);

        gl.BufferData(gl.ARRAY_BUFFER, vertices.len * @sizeOf(f32), &vertices[0], gl.STATIC_DRAW);
    }

    fn create_shaders(self: *renderer) void {
        self.vertex_shader = gl.CreateShader(gl.VERTEX_ARRAY);

        gl.ShaderSource(self.vertex_shader, 1, &vertex_shader_source[0], null);
    }

    pub fn destroy(self: *renderer) void {
        var vbos_to_delete = [_]c_uint{ self.vbo };
        gl.DeleteBuffers(1, vbos_to_delete[0..].ptr);

        _ = c.SDL_GL_DestroyContext(self.gl_context);
        gl.makeProcTableCurrent(null);

    }
};
