#include <Nashi/Nashi.hpp>

#include <iostream>

namespace Nashi {

namespace Helpers {

  IRenderer* getBackendClass() {
    switch(g_init.backend) {
      case LNP:
        return nullptr;
      case VULKAN:
        return std::get<VulkanRenderer*>(g_renderer);
      case DIRECTX12:
        return std::get<Direct3D12Renderer*>(g_renderer);
      case OPENGL:
        return std::get<OpenGLRenderer*>(g_renderer);
      case METAL:
        return nullptr;
      case WEBGPU:
        return nullptr;
      case NVN:
        return nullptr;
      case GNM:
        return nullptr;
      case SOFTWARE:
        return nullptr;
    }

    return nullptr;
  }
}

  Init g_init;
  std::variant<VulkanRenderer* , OpenGLRenderer* , Direct3D12Renderer*> g_renderer;

  bool init(Init init) {
    switch (init.backend) {
      case LNP: {
        break;
      }

      case VULKAN: {
        Uint32 extensionCount = 0;
        const char* const* instanceExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

        if (instanceExtensions == nullptr) {
            std::cerr << "SDL_Vulkan_GetInstanceExtensions failed: " << SDL_GetError() << "\n";
            SDL_DestroyWindow(init.SDL_window);
            SDL_Quit();
            return false;
        }

        int countExtensions = extensionCount + 1;  // +1 for VK_EXT_DEBUG_REPORT
        const char** extensions = static_cast<const char**>(SDL_malloc(countExtensions * sizeof(const char*)));
        extensions[0] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
        SDL_memcpy(&extensions[1], instanceExtensions, extensionCount * sizeof(const char*));

        g_renderer = new VulkanRenderer(extensions, countExtensions, init.SDL_window, init.SDL_event);
        VulkanRenderer* vk = std::get<VulkanRenderer*>(g_renderer); 

        vk->init();
      }

      case DIRECTX12:
      case OPENGL:
      case METAL:
      case WEBGPU:
      case NVN:
      case GNM:
      case SOFTWARE: {
        // Not yet implemented
        break;
      }

      default: {
        std::cerr << "Unknown backend selected.\n";
        return false;
      }
    }

    g_init = init;
    return true;
  }

  void createVertexBuffer() {}

  void createIndexBuffer() {}

  void createPipelineLayout() {}

  void draw() {
    Helpers::getBackendClass()->draw();
  }

  void resize() { 
      auto* backend = Helpers::getBackendClass();

      backend->resize();
  }

  void shutdown() {
    delete Helpers::getBackendClass();

  }

}
