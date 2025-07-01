#ifdef NASHI_USE_DIRECT3D12
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>


// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <chrono>

#include <renderer.hpp>

#define SDL_WINDOW_NAME "DirectX12 Window (nashi)"

using Microsoft::WRL::ComPtr;

#define CHECK_DX(expr)                                                \
    do {                                                              \
        HRESULT res = (expr);                                         \
        if (FAILED(res)) {                                            \
            if (res == DXGI_ERROR_DEVICE_REMOVED) {                  \
                HRESULT reason = m_dxDevice->GetDeviceRemovedReason(); \
                std::cerr << "Device removed reason: 0x" << std::hex << reason << std::endl; \
            }                                                         \
            std::ostringstream oss;                                   \
            oss << "DirectX12 error: HRESULT = 0x" << std::hex << res; \
            std::cerr << oss.str() << std::endl;                      \
            throw std::runtime_error(oss.str());                      \
        }                                                             \
    } while (0)

namespace Nashi {
	class Direct3D12Renderer : IRenderer {
		SDL_Window* m_window;

		HWND m_hwnd;
		RECT m_windowRect;

		bool m_dxVsync = true;
		bool m_dxTearingSupported = false;
		bool m_dxFullscreen;

		static const uint8_t m_dxNumFrames = 2;
		bool m_dxUseWarp = false;

		bool m_dxIsInitialized = false;

		// DirectX 12 Objects
		ComPtr<IDXGIFactory5> m_dxFactory5;
		ComPtr<IDXGIAdapter4> m_dxAdapter;
		ComPtr<ID3D12Device2> m_dxDevice;
		ComPtr<ID3D12CommandQueue> m_dxCommandQueue;

		ComPtr<IDXGISwapChain4> m_dxSwapChain;
		ComPtr<ID3D12DescriptorHeap> m_dxRTVDescriptorHeap;
		UINT m_dxRTVDescriptorSize;
		ComPtr<ID3D12Resource> m_dxBackBuffers[m_dxNumFrames];
		UINT m_dxCurrentBackBufferIndex;
		ComPtr<ID3D12CommandAllocator> m_dxCommandAllocators[m_dxNumFrames];
		ComPtr<ID3D12GraphicsCommandList> m_dxCommandList[m_dxNumFrames];

		ComPtr<ID3D12Fence> m_dxFence;
		uint64_t m_dxFenceValue = 0;
		uint64_t m_dxFrameFenceValues[m_dxNumFrames] = {};
		HANDLE m_dxFenceEvent;

#ifdef _DEBUG
		void enableDebugLayer();
#endif
		void createFactory();
		void createAdapter();
		void createDevice();
		void createCommandQueue();

		void checkTearingSupport();

		void createSwapChain();
		void createDescriptorHeap();
		void updateRenderTargetViews();

		void createCommandAllocators();
		void createCommandLists();

		void createSyncObjects();
		void createEventHandle();
		void signalFence();
		void waitForFenceValue(std::chrono::milliseconds duration = std::chrono::milliseconds::max());

		void flush();

	public:
		bool m_windowResized = false;
		Direct3D12Renderer(SDL_Window* window, HWND hwnd);

		void init();
		void draw();
		void cleanup();
	};
}
#endif