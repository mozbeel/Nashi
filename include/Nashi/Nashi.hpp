#pragma once

#include <renderer_d3d12.hpp>
#include <renderer_gl.hpp>
#include <renderer_mtl.hpp>
#include <renderer_vk.hpp>

#include <variant>

namespace Nashi {
  enum Backend {
    LNP, // Let Nashi Pick
    VULKAN,
    DIRECTX12,
    OPENGL,
    METAL,
    WEBGPU,
    NVN,
    GNM,
    SOFTWARE
  };

  struct Init {
    Backend backend;
    SDL_Window* SDL_window;
    void* nwh;
    void* ndt;
    SDL_Event SDL_event;
  };


  namespace Helpers {
    IRenderer* getBackendClass();
  };

  extern Init g_init;
  extern std::variant<VulkanRenderer* , OpenGLRenderer* , Direct3D12Renderer*> g_renderer;

  bool init(Init init);

  void draw();

  void resize();

  void shutdown();
}

