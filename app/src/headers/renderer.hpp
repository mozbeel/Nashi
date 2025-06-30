#ifndef NASHI_VR
#   include <SDL3/SDL.h>
#   include <SDL3/SDL_vulkan.h>
#endif

namespace Nashi {
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