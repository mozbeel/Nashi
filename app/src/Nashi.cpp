#include "renderer_vk.hpp"
#include <Nashi/Nashi.hpp>

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

  bool init(Init init) {
    switch (init.backend) {
      case LNP: {
        break;
      }

      case VULKAN: {
        unsigned int extensionCount = 0;
        
        if (!SDL_Vulkan_GetInstanceExtensions(&extensionCount)) {
          return false;
        }

        Uint32 count_instance_extensions;
        const char * const *instance_extensions = SDL_Vulkan_GetInstanceExtensions(&count_instance_extensions);

        if (instance_extensions == NULL) {
          std::cerr << "SDL_Vulkan_GetInstanceExtensions failed: " << SDL_GetError() << "\n";
          SDL_DestroyWindow(init.SDL_window);
          SDL_Quit();
          return false;
        }

        int countExtensions = count_instance_extensions + 1;
        SDL_malloc(countExtensions * sizeof(const char *));
        // Fix SDL_malloc cas:WEBGP:WEBGPUUt to correct type
        const char** extensions = static_cast<const char**>(SDL_malloc(countExtensions * sizeof(const char*)));  
        extensions[0] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
        SDL_memcpy(&extensions[1], instance_extensions, count_instance_extensions * sizeof(const char*)); 

        g_renderer = new VulkanRenderer(extensions, extensionCount, init.SDL_window, init.SDL_event);
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

  void draw() {
    Helpers::getBackendClass()->draw();
  }

  void shutdown() {
    Helpers::getBackendClass()->cleanup();
    delete Helpers::getBackendClass();

  }

}
