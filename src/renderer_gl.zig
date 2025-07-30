const std = @import("std");
const gl = @import("gl");
const c = @import("c.zig").c;

pub const renderer = struct {
    gl_context : c.SDL_GLContext = undefined,
    window : ?*c.SDL_Window = undefined,
    vbo: c_uint = 0,
    vao: c_uint = 0,
    ebo: c_uint = 0,
    vertex_shader: c_uint = 0,
    fragment_shader: c_uint = 0,
    shader_program: c_uint = 0,

    var procs : gl.ProcTable = undefined; // HAS to be defined as var for some reason here
    const vertices = [_]f32{
        0.5, 0.5, 0.0, // top right
        0.5, -0.5, 0.0, // bottom right
        -0.5, -0.5, 0.0, // bottom left
        -0.5, 0.5, 0.0, // top left
    };

    const indices = [_]c_uint{
        0, 1, 3,
        1, 2, 3,
    };
   
    // Zig terminates every string automatically which breaks if broken into multiple lines, so I'm using a multi-line string here for readability
    const vertex_shader_source = [_][*]const u8{
        \\#version 330 core
        \\layout (location = 0) in vec3 aPos;
        \\
        \\void main() {
        \\  gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
        \\}
        \\
    };

    const fragment_shader_source = [_][*]const u8{
        \\#version 330 core
        \\out vec4 FragColor;
        \\
        \\void main() {
        \\  FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
        \\}
        \\
    };


    pub fn init(window: ?*c.SDL_Window) !renderer {
        var self : renderer = .{
            .window = window,
        };
        try self.prepare_opengl();
       
        self.create_shaders();
        self.create_buffers();
    
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

    fn create_buffers(self: *renderer) void {
        var vao_storage: [1]c_uint = .{ 0 };
        gl.GenVertexArrays(1, vao_storage[0..].ptr);
        self.vao = vao_storage[0];

        var buffer_storage: [2]c_uint = .{ 0, 0 };
        gl.GenBuffers(2, buffer_storage[0..].ptr);
        self.vbo = buffer_storage[0];
        self.ebo = buffer_storage[1];

        gl.BindVertexArray(self.vao);

        gl.BindBuffer(gl.ARRAY_BUFFER, self.vbo);
        gl.BufferData(gl.ARRAY_BUFFER, @sizeOf(f32) * vertices.len, &vertices[0], gl.STATIC_DRAW);

        gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, self.ebo);
        gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, @sizeOf(c_uint) * vertices.len, &indices[0], gl.STATIC_DRAW);
        
        gl.VertexAttribPointer(0, 3, gl.FLOAT, gl.FALSE, 3 * @sizeOf(f32), 0);
        gl.EnableVertexAttribArray(0);
    }

    fn create_shaders(self: *renderer) void {
        self.vertex_shader = create_shader(gl.VERTEX_SHADER, vertex_shader_source[0..]);

        self.fragment_shader = create_shader(gl.FRAGMENT_SHADER, fragment_shader_source[0..]);

        self.shader_program = gl.CreateProgram();
        gl.AttachShader(self.shader_program, self.vertex_shader);
        gl.AttachShader(self.shader_program, self.fragment_shader);
        gl.LinkProgram(self.shader_program);

        var success: c_int = 0;
        var info_log: [512]u8 = undefined;

        gl.GetProgramiv(self.shader_program, gl.LINK_STATUS, &success);
        if (success == 0) {
            gl.GetProgramInfoLog(self.shader_program, 512, null, info_log[0..].ptr);
            std.log.err("ERROR::LINKING::SHADER_PROGRA\n{s}\n", .{ info_log });
            std.process.exit(1);
        }
        gl.DeleteShader(self.vertex_shader);
        gl.DeleteShader(self.fragment_shader);
    }

    fn create_shader(shader_type: c_uint, shader_source: []const [*]const u8) c_uint {
        const shader = gl.CreateShader(shader_type);
        gl.ShaderSource(shader, 1, shader_source[0..].ptr, null);

        gl.CompileShader(shader);

        var success : c_int = 0;
        var info_log: [512]u8 = undefined;

        gl.GetShaderiv(shader, gl.COMPILE_STATUS, &success);
        if (success == 0) {
            gl.GetShaderInfoLog(shader, 512, null, info_log[0..].ptr);
            std.log.err("ERROR::SHADER::COMPILATION_FAILED\n{s}\n", .{ info_log });
            std.process.exit(1);
        }

        return shader;
    }

    pub fn draw(self: *renderer) void {
        gl.ClearColor(128.0/255.0, 30.0/255.0, 1, 1);
        gl.Clear(gl.COLOR_BUFFER_BIT);

        gl.UseProgram(self.shader_program);
        gl.BindVertexArray(self.vao);
        
        gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, self.ebo);

        gl.DrawElements(gl.TRIANGLES, 6, gl.UNSIGNED_INT, 0);
        
        _ = c.SDL_GL_SwapWindow(self.window.?);

    }


    pub fn destroy(self: *renderer) void {
        var vbos_to_delete = [_]c_uint{ self.vbo };
        gl.DeleteBuffers(1, vbos_to_delete[0..].ptr);

        gl.DeleteProgram(self.shader_program);

        _ = c.SDL_GL_DestroyContext(self.gl_context);
        gl.makeProcTableCurrent(null);

    }
};
