#ifdef NASHI_USE_VULKAN
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <fstream>
#include <algorithm>
#include <array>
#include <chrono>

#include <renderer.hpp>

#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h>
#  include <vulkan/vulkan_win32.h>
#endif

#define SDL_WINDOW_NAME "Vulkan Window (nashi)"

inline auto srgbToLinear = [](float c) {
    return (c <= 0.04045f) ? c / 12.92f : pow((c + 0.055f) / 1.055f, 2.4f);
    };
namespace Nashi {

    const std::vector<const char*> validationLayers = {
      "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifndef _DEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

#define CHECK_VK(expr) \
  do { \
    VkResult res = (expr); \
    if (res != VK_SUCCESS) { \
      std::ostringstream oss; \
      oss << "Vulkan error: " << res; \
      std::cout << oss.str() << std::endl; \
      throw std::runtime_error(oss.str()); \
    } \
  } while (0)


#define SHADER_ENTRY_POINT "main"

    const int MAX_FRAMES_IN_FLIGHT = 2;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
    };


    class VulkanRenderer : IRenderer {
    private:
        VkInstance m_vkInstance;
        VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_vkDevice;
        VkQueue m_vkGraphicsQueue;
        VkSurfaceKHR m_vkSurface;
        VkQueue m_vkPresentQueue;

        VkSwapchainKHR m_vkSwapChain;
        std::vector<VkImage> m_vkSwapChainImages;
        VkFormat m_vkSwapChainImageFormat;
        VkExtent2D m_vkSwapChainExtent;
        std::vector<VkImageView> m_vkSwapChainImageViews;
        std::vector<VkFramebuffer> m_vkSwapChainFramebuffers;
        
        VkPipelineLayout m_vkPipelineLayout;
        VkDescriptorSetLayout m_vkDescriptorSetLayout;

        VkRenderPass m_vkRenderPass;
        VkPipeline m_vkGraphicsPipeline;

        VkCommandPool m_vkCommandPool;

        VkDeviceSize m_vkVertexBufferSize;
        VkBuffer m_vkCombinedBuffer;
        VkDeviceMemory m_vkCombinedBufferMemory;

        std::vector<VkBuffer> m_vkUniformBuffers;
        std::vector<VkDeviceMemory> m_vkUniformBuffersMemory;
        std::vector<void*> m_vkUniformBuffersMapped;
        VkDescriptorPool m_vkDescriptorPool;
        std::vector<VkDescriptorSet> m_vkDescriptorSets;

        std::vector<VkCommandBuffer> m_vkCommandBuffers;

        std::vector<VkSemaphore> m_vkImageAvailableSemaphores;
        std::vector<VkSemaphore> m_vkRenderFinishedSemaphores;
        std::vector<VkFence> m_vkInFlightFences;

        uint32_t currentFrame = 0;

        const char** m_extraExtensions;
        int m_extraExtensionsCount;

        SDL_Window* m_window;
        SDL_Event m_event;

        void createInstance();
        bool checkValidationSupport();
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

        std::vector<const char*> getRequiredExtensions();
        void createSurface();
        void pickPhysicalDevice();

        int rateDeviceSuitability(VkPhysicalDevice device);

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        void createLogicalDeivce();

        SwapChainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        void createSwapChain();
        void createImageViews();
        void recreateSwapChain();
        void cleanupSwapChain();
        void cleanupSyncObjects();

        void createRenderPass();

        void createDescriptorSetLayout();
        void createGraphicsPipeline();
        VkShaderModule createShaderModule(const std::vector<char>& code);

        void createFramebuffers();
        void createCommandPool();

        const std::vector<Vertex> m_vertices = {
            // Front face
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // 0
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}}, // 1
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // 2
            {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}, // 3

            // Back face
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}}, // 4
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}}, // 5
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // 6
            {{-0.5f,  0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}}, // 7
        };

        const std::vector<uint16_t> m_indices = {
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


        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
            VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        void createCombinedBuffer();

        void createUniformBuffers();
        void updateUniformBuffer(uint32_t currentImage);
        void createDescriptorPool();
        void createDescriptorSets();

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        void createCommandBuffers();

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void createSyncObjects();

    public:
        bool m_windowResized;
        VulkanRenderer(const char** m_extraExtensions, int m_extraExtensionsCount, SDL_Window* window, SDL_Event event);
        void init();
        void draw();
        void cleanup();
    };
};


#endif