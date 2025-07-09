#include <Nashi/Nashi.hpp>

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

  Nashi::Init init;
  init.backend = Nashi::VULKAN;
  init.SDL_event = event;
  init.SDL_window = window;

  Nashi::init(init);

  while(running) {
    while (SDL_PollEvent(&event)) {
      switch(event.type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        case SDL_EVENT_QUIT:
          running = false;
          break;
        case SDL_EVENT_WINDOW_RESIZED:
          Nashi::resize();
          break;
      }
    }
    Nashi::draw();
  }
  Nashi::shutdown();

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
 
