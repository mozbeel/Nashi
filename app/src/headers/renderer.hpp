#ifndef NASHI_VR
#   include <SDL3/SDL.h>
#   include <SDL3/SDL_vulkan.h>
#endif

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::cout << "failed to open file : " + filename << std::endl;
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    if (!file) {
        throw std::runtime_error("failed to read entire file: " + filename);
    }

    file.close();

    return buffer;
}

static std::vector<char> readFileText(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("failed to open file");

    file.seekg(0, std::ios::end);
    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(static_cast<size_t>(fileSize) + 1);
    file.read(buffer.data(), fileSize);
    buffer[fileSize] = '\0';

    if (!file) {
        throw std::runtime_error("failed to read entire file");
    }

    file.close();

    return buffer;
}

namespace Nashi {
    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

	class IRenderer {
	public:
		bool m_windowResized;
		virtual ~IRenderer() = default;
		IRenderer() = default;

		virtual void init() = 0;
		virtual void draw() = 0;
		virtual void cleanup() = 0;
	};

}