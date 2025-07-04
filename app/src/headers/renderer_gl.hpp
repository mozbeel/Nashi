#ifdef NASHI_USE_OPENGL
#include <glad/glad.h>
#include <renderer.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>;

#define SDL_WINDOW_NAME "OpenGL Window (nashi)"

namespace Nashi {
	class OpenGLRenderer : IRenderer {
		SDL_GLContext m_glContext;
		SDL_Window* m_window;
		SDL_Event m_event;

		int m_windowWidth, m_windowHeight;

		std::vector<float> m_vertices = {
			// pos.xyz, color.rgb
			-0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,
			 0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
			-0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f,
			-0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
			 0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
			 0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 1.0f,
			-0.5f,  0.5f, -0.5f, 0.5f, 0.5f, 0.5f,
		};

		const std::vector<unsigned int> m_indices = {
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
			4, 5, 1, 1, 0, 4
		};

		unsigned int m_glVBO;
		unsigned int m_glVAO;
		unsigned int m_glEBO;
		
		unsigned int m_glUBO;
		unsigned int m_glUBOBindingPoint = 0;

		unsigned int m_glVertexShader;
		unsigned int m_glFragmentShader;
		unsigned int m_glShaderProgram;

		void resizeWindow();
		unsigned int createShader(GLenum shaderType, const std::string& filename);
		void createShaderProgram();

		void deleteShaders();
		void createVertexInputState();

		void createBuffers();
		void createVertexBuffer();
		void createIndexBuffer();

		void createUniformBuffer();
		void updateUniformBuffer();

		void createVertexAttributes();
	public:
		bool m_windowResized = false;
		OpenGLRenderer(SDL_Window* window, SDL_Event event);

		void init();
		void draw();
		void cleanup();
	};

}
#endif