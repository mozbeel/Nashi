const std = @import("std");
const builtin = @import("builtin");

const entry = @import("entry.zig");
const c = @import("c.zig").c;
const gl = @import("gl");
const za = @import("zalgebra");

pub const renderer = struct {
    gl_context : c.SDL_GLContext = undefined,
    window : ?*c.SDL_Window = undefined,
    vbo: c_uint = 0,
    vao: c_uint = 0,
    ebo: c_uint = 0,
    vertex_shader: c_uint = 0,
    fragment_shader: c_uint = 0,
    shader_program: c_uint = 0,
    mvp_loc: c_int = 0,
    window_width: c_int = 0,
    window_height: c_int = 0,

    var procs : gl.ProcTable = undefined; // HAS to be defined as var for some reason here
    const vertices = [_]f32{
        //  x      y      z     r    g    b
        -0.5, -0.5,  0.5, 1.0, 0.0, 0.0,
         0.5, -0.5,  0.5, 0.0, 1.0, 0.0,
         0.5,  0.5,  0.5, 0.0, 0.0, 1.0,
        -0.5,  0.5,  0.5, 1.0, 1.0, 1.0,
        -0.5, -0.5, -0.5, 1.0, 1.0, 0.0,
         0.5, -0.5, -0.5, 0.0, 1.0, 1.0,
         0.5,  0.5, -0.5, 1.0, 0.0, 1.0,
        -0.5,  0.5, -0.5, 0.5, 0.5, 0.5,

    };

    const indices = [_]c_uint{
        // Front face
        0, 1, 2, 2, 3, 0,
        // Right face
        1, 5, 6, 6, 2, 1,
        // Back face
        5, 4, 7, 7, 6, 5,
        // Left face
        4, 0, 3, 3, 7, 4,
        // Top face
        3, 2, 6, 6, 7, 3,
        // Bottom face
        4, 5, 1, 1, 0, 4,
    };
   
    // Zig terminates every string automatically which breaks if broken into multiple lines, so I'm using a multi-line string here for readability
    const vertex_shader_source = if(
        builtin.target.abi == .android 
        or builtin.target.os.tag == .emscripten 
        or builtin.target.os.tag == .ios) [_][*]const u8{
        \\#version 300 es
        \\layout (location = 0) in vec3 aPos;
        \\layout (location = 1) in vec3 aColor;
        \\
        \\out vec3 vertexColor;
        \\
        \\uniform mat4 mvp;
        \\
        \\void main() {
        \\  gl_Position = mvp *vec4(aPos, 1.0);
        \\  vertexColor = aColor;
        \\}
        \\
    } else [_][*]const u8{
        \\#version 330 core
        \\layout (location = 0) in vec3 aPos;
        \\layout (location = 1) in vec3 aColor;
        \\
        \\out vec3 vertexColor;
        \\
        \\uniform mat4 mvp;
        \\
        \\void main() {
        \\  gl_Position = mvp * vec4(aPos, 1.0);
        \\  vertexColor = aColor;
        \\}
        \\
    };

    const fragment_shader_source = if(
        builtin.target.abi == .android 
        or builtin.target.os.tag == .emscripten 
        or builtin.target.os.tag == .ios) [_][*]const u8{
        \\#version 300 es
        \\precision mediump float;
        \\precision mediump int;
        \\
        \\in vec3 vertexColor;
        \\out vec4 FragColor;
        \\
        \\void main() {
        \\  FragColor = vec4(vertexColor, 1.0f);
        \\}
        \\
    } else [_][*]const u8{
        \\#version 330 core
        \\
        \\in vec3 vertexColor;
        \\out vec4 FragColor;
        \\
        \\void main() {
        \\  FragColor = vec4(vertexColor, 1.0f);
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
        self.create_uniforms();
    
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

        if(!c.SDL_GetWindowSizeInPixels(self.window, &self.window_width, &self.window_height)) return error.CouldntGetWindowSize;
        gl.Viewport(0, 0, @intCast(self.window_width), @intCast(self.window_height));

        gl.Enable(gl.DEPTH_TEST);
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
        gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, @sizeOf(c_uint) * indices.len, &indices[0], gl.STATIC_DRAW);
        
        gl.VertexAttribPointer(0, 3, gl.FLOAT, gl.FALSE, 6 * @sizeOf(f32), 0);
        gl.EnableVertexAttribArray(0);

        gl.VertexAttribPointer(1, 3, gl.FLOAT, gl.FALSE, 6 * @sizeOf(f32), 3 * @sizeOf(f32));
        gl.EnableVertexAttribArray(1);
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

    fn create_uniforms(self: *renderer) void {
        self.mvp_loc = gl.GetUniformLocation(self.shader_program, "mvp");
    }

    pub fn draw(self: *renderer) !void {
        gl.ClearColor(128.0/255.0, 30.0/255.0, 1, 1);
        
        var to_clear : c_uint = gl.COLOR_BUFFER_BIT;
        to_clear |= gl.DEPTH_BUFFER_BIT;
        gl.Clear(to_clear);

        gl.UseProgram(self.shader_program);
        try self.update_uniforms();
        
        gl.BindVertexArray(self.vao);
        
        gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, self.ebo);

        gl.DrawElements(gl.TRIANGLES, indices.len, gl.UNSIGNED_INT, 0);
        
        _ = c.SDL_GL_SwapWindow(self.window.?);

    }

    fn update_uniforms(self: *renderer) !void {
        const w: f32 = @floatFromInt(self.window_width);
        const h: f32 = @floatFromInt(self.window_height);

        const projection = za.perspective(45.0, w / h, 0.1, 1000.0);
        const view = za.lookAt(za.Vec3.new(1.5, 1.5, -3.0), za.Vec3.zero(), za.Vec3.up());
        var model = za.Mat4.fromTranslate(za.Vec3.new(0.0, 0.0, 0.0));

        const now = std.time.microTimestamp();
        const elapsed_ms = now - entry.start_time;
        model = model.rotate(@as(f32, @floatCast(@as(f64, @floatFromInt(elapsed_ms))))/1_000_0, za.Vec3.new(0.0, 1.0, 0.0));

        const mvp = za.Mat4.mul(projection, view.mul(model));

        gl.UniformMatrix4fv(self.mvp_loc, 1, gl.FALSE, @as([*c]const f32, @ptrCast(&mvp)));
    }

    pub fn resized_window(self: *renderer) !void {
        if(!c.SDL_GetWindowSizeInPixels(self.window, &self.window_width, &self.window_height)) return error.CouldntGetWindowSize;
    
        gl.Viewport(0, 0, @intCast(self.window_width), @intCast(self.window_height));
    }


    pub fn destroy(self: *renderer) void {
        var vbos_to_delete = [_]c_uint{ self.vbo };
        gl.DeleteBuffers(1, vbos_to_delete[0..].ptr);

        gl.DeleteProgram(self.shader_program);

        _ = c.SDL_GL_DestroyContext(self.gl_context);
        gl.makeProcTableCurrent(null);

    }
};
