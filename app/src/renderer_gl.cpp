#ifdef NASHI_USE_OPENGL
#include <renderer_gl.hpp>
#include <filesystem>

namespace Nashi {
	OpenGLRenderer::OpenGLRenderer(SDL_Window* window, SDL_Event event) {
		this->m_window = window;
		this->m_event = event;
	}

	void OpenGLRenderer::resizeWindow() {
		SDL_GetWindowSizeInPixels(m_window, &m_windowWidth, &m_windowHeight);
		glViewport(0, 0, m_windowWidth, m_windowHeight);

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

	void OpenGLRenderer::deleteShaders() {
		glDeleteShader(m_glVertexShader);
		glDeleteShader(m_glFragmentShader);
	}

	void OpenGLRenderer::createVertexInputState() {
		glGenVertexArrays(1, &m_glVAO);
		glBindVertexArray(m_glVAO);
	}

	void OpenGLRenderer::createBuffers() {
		createVertexBuffer();
		createIndexBuffer();
	}
	
	void OpenGLRenderer::createVertexBuffer() {
		glGenBuffers(1, &m_glVBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_glVBO);
		glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(float), m_vertices.data(), GL_STATIC_DRAW);
	}

	void OpenGLRenderer::createIndexBuffer() {
		glGenBuffers(1, &m_glEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);
	}

	void OpenGLRenderer::createUniformBuffer() {
		glGenBuffers(1, &m_glUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, m_glUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 3, nullptr, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		glBindBufferBase(GL_UNIFORM_BUFFER, m_glUBOBindingPoint, m_glUBO);	
	}

	void OpenGLRenderer::createVertexAttributes() {
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3*sizeof(float)));
		glEnableVertexAttribArray(1);
	}

	void OpenGLRenderer::init() {
		m_glContext = SDL_GL_CreateContext(m_window);
		if (!m_glContext) {
			std::cout << "Failed to create GL context: " << SDL_GetError() << std::endl;
			return;
		}

		if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
			std::cout << "Failed to initialize GLAD" << std::endl;
			return;

		}
		resizeWindow();

		glEnable(GL_DEPTH_TEST);
	
		std::filesystem::path vertShaderPath = std::filesystem::current_path() / "shaders" / "basic.vert";
		std::filesystem::path fragShaderPath = std::filesystem::current_path() / "shaders" / "basic.frag";
		m_glVertexShader = createShader(GL_VERTEX_SHADER, vertShaderPath.string());
		m_glFragmentShader = createShader(GL_FRAGMENT_SHADER, fragShaderPath.string());

		createShaderProgram();
		deleteShaders();

		createVertexInputState();
		createBuffers();
		createVertexAttributes();

		createUniformBuffer();

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	
	void OpenGLRenderer::draw() {
		if (m_windowResized) {
			resizeWindow();
			m_windowResized = false;
		}
		glClearColor(129.0f / 255.0f, 186.0f / 255.0f, 219.0f / 255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_glShaderProgram);

		updateUniformBuffer();

		glBindVertexArray(m_glVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glEBO);

		glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, 0);

		SDL_GL_SwapWindow(m_window);
	}

	void OpenGLRenderer::updateUniformBuffer() {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo;
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f,
			0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f),
			m_windowWidth / (float)m_windowHeight, 0.1f,
			10.0f);

		glBindBuffer(GL_UNIFORM_BUFFER, m_glUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ubo), &ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void OpenGLRenderer::cleanup() {

		glDeleteVertexArrays(1, &m_glVAO);

		const unsigned int removedBuffers[] = { m_glEBO, m_glVBO };

		glDeleteBuffers(sizeof(removedBuffers) / sizeof(removedBuffers[0]), removedBuffers);
		glDeleteProgram(m_glShaderProgram);
		SDL_GL_DestroyContext(m_glContext);
	}
}
#endif