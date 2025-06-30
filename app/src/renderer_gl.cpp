#ifdef NASHI_USE_OPENGL
#include <renderer_gl.hpp>

namespace Nashi {
	OpenGLRenderer::OpenGLRenderer(SDL_Window* window, SDL_Event event) {
		this->m_window = window;
		this->m_event = event;
	}

	void OpenGLRenderer::resizeWindow() {
		int width = 0, height = 0;
		SDL_GetWindowSizeInPixels(m_window, &width, &height);
		glViewport(0, 0, width, height);

	}

	void OpenGLRenderer::init() {
		SDL_GL_CreateContext(m_window);
		if (!m_glContext) {
			std::cout << "Failed to create GL context: " << SDL_GetError() << std::endl;
			return;
		}

		if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
			std::cout << "Failed to initialize GLAD" << std::endl;
			return;

		}
		resizeWindow();
	}
	
	void OpenGLRenderer::draw() {
		if (m_windowResized) {
			resizeWindow();
			m_windowResized = false;
		}
		glClearColor(129.0f / 255.0f, 186.0f / 255.0f, 219.0f / 255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		SDL_GL_SwapWindow(m_window);
	}

	void OpenGLRenderer::cleanup() {
	}
}
#endif