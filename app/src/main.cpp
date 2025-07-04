#ifdef NASHI_USE_VULKAN
#   include <renderer_vk.hpp>
#elif NASHI_USE_OPENGL
#   include <renderer_gl.hpp>
#elif NASHI_USE_DIRECT3D12
#   include <renderer_d3d12.hpp>
#endif


#include <iostream>
#include <vector>

int main() {
  if(SDL_Init(SDL_INIT_VIDEO) == false) {
    return EXIT_FAILURE;
  }

#ifdef NASHI_USE_OPENGL
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#endif

  SDL_Window* window = SDL_CreateWindow(SDL_WINDOW_NAME, 1280, 720, 
      SDL_WINDOW_RESIZABLE |
    #ifdef NASHI_USE_VULKAN
      SDL_WINDOW_VULKAN
    #elif NASHI_USE_OPENGL
      SDL_WINDOW_OPENGL
    #elif NASHI_USE_METAL
      SDL_WINDOW_METAL
    #else
      0
    #endif
  );
  if (!window) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
    SDL_Quit();
    return EXIT_FAILURE;
  }

  bool running = true;
  SDL_Event event;
  memset(&event, 0, sizeof(event));
#ifdef NASHI_USE_VULKAN
  unsigned int extensionCount = 0;
  if (!SDL_Vulkan_GetInstanceExtensions(&extensionCount)) {
      // handle error
  }

  Uint32 count_instance_extensions;
  const char * const *instance_extensions = SDL_Vulkan_GetInstanceExtensions(&count_instance_extensions);

  if (instance_extensions == NULL) {
    std::cerr << "SDL_Vulkan_GetInstanceExtensions failed: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_FAILURE;
  }

  int countExtensions = count_instance_extensions + 1;
  SDL_malloc(countExtensions * sizeof(const char *));
  // Fix SDL_malloc cast to correct type
  const char** extensions = static_cast<const char**>(SDL_malloc(countExtensions * sizeof(const char*)));  
  extensions[0] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
  SDL_memcpy(&extensions[1], instance_extensions, count_instance_extensions * sizeof(const char*)); 

  Nashi::VulkanRenderer* vkRenderer = new Nashi::VulkanRenderer(extensions, countExtensions, window, event);
  vkRenderer->init();
#elif NASHI_USE_OPENGL
  Nashi::OpenGLRenderer* openGLRenderer = new Nashi::OpenGLRenderer(window, event);
  openGLRenderer->init();

#elif NASHI_USE_DIRECT3D12
  HWND hwnd = (HWND) SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

  Nashi::Direct3D12Renderer* direct3D12Renderer = new Nashi::Direct3D12Renderer(window, event, hwnd);

  direct3D12Renderer->init();
#endif


  while(running) {
    while (SDL_PollEvent(&event)) {
      switch(event.type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        case SDL_EVENT_QUIT:
          running = false;
          break;
        case SDL_EVENT_WINDOW_RESIZED:
#ifdef NASHI_USE_VULKAN
          vkRenderer->m_windowResized = true;
#elif NASHI_USE_OPENGL
          openGLRenderer->m_windowResized = true;
#elif NASHI_USE_DIRECT3D12
          direct3D12Renderer->m_windowResized = true;
#endif
          break;
      }
    }
#ifdef NASHI_USE_VULKAN
    vkRenderer->draw();
#elif NASHI_USE_OPENGL
    openGLRenderer->draw();
#elif NASHI_USE_DIRECT3D12
    direct3D12Renderer->draw();
#endif

  }
#ifdef NASHI_USE_VULKAN
  vkRenderer->cleanup();
#elif NASHI_USE_OPENGL
  openGLRenderer->cleanup();
#elif NASHI_USE_DIRECT3D12
  direct3D12Renderer->cleanup();
#endif

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}