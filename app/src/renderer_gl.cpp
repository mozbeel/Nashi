#ifdef NASHI_USE_OPENGL
#include <renderer_gl.hpp>
#include <filesystem>

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

	unsigned int OpenGLRenderer::createShader(GLenum shaderType, const std::string& filename) {
		unsigned int shader = glCreateShader(shaderType);
		auto shaderData = readFileText(filename);
		const char* shaderSource = shaderData.data();
		glShaderSource(shader, 1, &shaderSource, NULL);
		glCompileShader(shader);

		int success;
		char infoLog[512];
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" <<
				infoLog << std::endl;
		}

		return shader;
	}

	void OpenGLRenderer::createShaderProgram() {
		m_glShaderProgram = glCreateProgram();
		glAttachShader(m_glShaderProgram, m_glVertexShader);
		glAttachShader(m_glShaderProgram, m_glFragmentShader);
		glLinkProgram(m_glShaderProgram);

		int success;
		char infoLog[512];
		glGetProgramiv(m_glShaderProgram, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(m_glShaderProgram, 512, NULL, infoLog);
			std::cout << "ERROR::SHADERPROGRAM::CREATION_FAILED\n" <<
				infoLog << std::endl;
		}
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
	
		std::filesystem::path vertShaderPath = std::filesystem::current_path() / "shaders" / "basic.vert";
		std::filesystem::path fragShaderPath = std::filesystem::current_path() / "shaders" / "basic.frag";
		m_glVertexShader = createShader(GL_VERTEX_SHADER, vertShaderPath.string());
		m_glFragmentShader = createShader(GL_FRAGMENT_SHADER, fragShaderPath.string());

		createShaderProgram();
		glDeleteShader(m_glVertexShader);
		glDeleteShader(m_glFragmentShader);

		glGenVertexArrays(1, &m_glVAO);
		glGenBuffers(1, &m_glVBO);

		glBindVertexArray(m_glVAO);

		glBindBuffer(GL_ARRAY_BUFFER, m_glVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(m_glVertices), m_glVertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	
	void OpenGLRenderer::draw() {
		if (m_windowResized) {
			resizeWindow();
			m_windowResized = false;
		}
		glClearColor(129.0f / 255.0f, 186.0f / 255.0f, 219.0f / 255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(m_glShaderProgram);
		glBindVertexArray(m_glVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		SDL_GL_SwapWindow(m_window);
	}

	void OpenGLRenderer::cleanup() {

	}
}
#endif