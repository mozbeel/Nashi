#ifdef NASHI_USE_OPENGL
#include <glad/glad.h>
#include <renderer.hpp>

#include <iostream>

#define SDL_WINDOW_NAME "OpenGL Window (nashi)"

namespace Nashi {
	class OpenGLRenderer : IRenderer {
		SDL_GLContext m_glContext;
		SDL_Window* m_window;
		SDL_Event m_event;

		float m_glVertices[9] = {
			-0.5f, -0.5f, 0.0f,
			0.5f, -0.5f, 0.0f,
			0.0f, 0.5f, 0.0f
		};
		unsigned int m_glVBO;
		unsigned int m_glVAO;

		unsigned int m_glVertexShader;
		unsigned int m_glFragmentShader;
		unsigned int m_glShaderProgram;

		void resizeWindow();
		unsigned int createShader(GLenum shaderType, const std::string& filename);
		void createShaderProgram();
	public:
		bool m_windowResized = false;
		OpenGLRenderer(SDL_Window* window, SDL_Event event);

		void init();
		void draw();
		void cleanup();
	};

}
#endif