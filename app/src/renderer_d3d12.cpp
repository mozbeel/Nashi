#ifdef NASHI_USE_DIRECT3D12
#include <renderer_d3d12.hpp>

namespace Nashi {
	Direct3D12Renderer::Direct3D12Renderer(SDL_Window* window, HWND hwnd) {
		this->m_window = window;
		this->m_hwnd = hwnd;
	}
#ifdef _DEBUG
	void Direct3D12Renderer::enableDebugLayer() {
		ComPtr<ID3D12Debug> debugInterface;
		CHECK_DX(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
		debugInterface->EnableDebugLayer();
		
	}
#endif
	void Direct3D12Renderer::createFactory() {
		ComPtr<IDXGIFactory4> dxFactory4;
		UINT createFactoryFlags = 0;
#ifdef _DEBUG
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
		CHECK_DX(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxFactory4)));
		CHECK_DX(dxFactory4.As(&m_dxFactory5));
	}

	void Direct3D12Renderer::createAdapter() {
		ComPtr<IDXGIAdapter1> dxAdapter1;

		if (m_dxUseWarp) {
			CHECK_DX(m_dxFactory5->EnumWarpAdapter(IID_PPV_ARGS(&dxAdapter1)));
			CHECK_DX(dxAdapter1.As(&m_dxAdapter));
		}
		else {
			SIZE_T maxDedicatedVideoMemory = 0;
			for (UINT i = 0; m_dxFactory5->EnumAdapters1(i, &dxAdapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
				DXGI_ADAPTER_DESC1 dxAdapterDesc1;
				dxAdapter1->GetDesc1(&dxAdapterDesc1);

				if ((dxAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
					SUCCEEDED(D3D12CreateDevice(dxAdapter1.Get(), D3D_FEATURE_LEVEL_12_0,
						__uuidof(ID3D12Device), nullptr)) && dxAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory) {
					maxDedicatedVideoMemory = dxAdapterDesc1.DedicatedVideoMemory;
					CHECK_DX(dxAdapter1.As(&m_dxAdapter));
				}
			}
		}
	}
	void Direct3D12Renderer::createDevice() {
		CHECK_DX(D3D12CreateDevice(m_dxAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_dxDevice)));

#ifdef _DEBUG
		ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(m_dxDevice.As(&infoQueue))) {
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

			D3D12_MESSAGE_SEVERITY severities[] = {
				D3D12_MESSAGE_SEVERITY_INFO
			};

			D3D12_MESSAGE_ID denyIds[] = {
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
			};

			D3D12_INFO_QUEUE_FILTER newFilter{};
			newFilter.DenyList.NumSeverities = std::size(severities);
			newFilter.DenyList.pSeverityList = severities;
			newFilter.DenyList.NumIDs = std::size(denyIds);
			newFilter.DenyList.pIDList = denyIds;

			CHECK_DX(infoQueue->PushStorageFilter(&newFilter));
		}
#endif
	}

	void Direct3D12Renderer::createCommandQueue() {
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		CHECK_DX(m_dxDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_dxCommandQueue)));
	}

	void Direct3D12Renderer::checkTearingSupport() {
		m_dxTearingSupported = false;
		HRESULT hr = m_dxFactory5->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
			&m_dxTearingSupported,
			sizeof(m_dxTearingSupported));

		if (FAILED(hr)) {
			m_dxTearingSupported = false;
		}
	}
	
	void Direct3D12Renderer::createSwapChain() {
		for (int i = 0; i < m_dxNumFrames; ++i) {
			m_dxBackBuffers[i].Reset();
		}

		m_dxRTVDescriptorHeap.Reset();
		m_dxSwapChain.Reset();

		UINT createFactoryFlags = 0;
#ifdef _DEBUG
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
		int width = 0, height = 0;
		SDL_GetWindowSizeInPixels(m_window, &width, &height);
		
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
		swapChainDesc.Width = width;
		swapChainDesc.Height = height;
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc = { 1, 0 };
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = m_dxNumFrames;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc.Flags = m_dxTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		ComPtr<IDXGISwapChain1> swapChain1;
		CHECK_DX(m_dxFactory5->CreateSwapChainForHwnd(m_dxCommandQueue.Get(),
			m_hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));

		CHECK_DX(m_dxFactory5->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

		CHECK_DX(swapChain1.As(&m_dxSwapChain));

		createDescriptorHeap();
		updateRenderTargetViews();
	}

	void Direct3D12Renderer::createDescriptorHeap() {
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
		descriptorHeapDesc.NumDescriptors = m_dxNumFrames;;
		descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		CHECK_DX(m_dxDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_dxRTVDescriptorHeap)));
	}

	void Direct3D12Renderer::updateRenderTargetViews() {
		UINT rtvDescriptorSize = m_dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_dxRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		for (int i = 0; i < m_dxNumFrames; ++i) {
			ComPtr<ID3D12Resource> backBuffer;
			CHECK_DX(m_dxSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

			m_dxDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

			m_dxBackBuffers[i] = backBuffer;

			rtvHandle.ptr += rtvDescriptorSize;
		}
	}

	void Direct3D12Renderer::createCommandAllocators() {
		for (int i = 0; i < m_dxNumFrames; ++i) {
			CHECK_DX(m_dxDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_dxCommandAllocators[i])));
		}
	}

	void Direct3D12Renderer::createCommandLists() {
		for (int i = 0; i < m_dxNumFrames; ++i) {
			CHECK_DX(m_dxDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_dxCommandAllocators[i].Get(), nullptr, IID_PPV_ARGS(&m_dxCommandList[i])));
			
			CHECK_DX(m_dxCommandList[i]->Close());
		}

	}

	void Direct3D12Renderer::createSyncObjects() {
		CHECK_DX(m_dxDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_dxFence)));
	}

	void Direct3D12Renderer::createEventHandle() {
		m_dxFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		assert(m_dxFenceEvent && "Failed to create fence event");
	}

	void Direct3D12Renderer::signalFence() {
		uint64_t fenceValueForSignal = ++m_dxFenceValue;
		CHECK_DX(m_dxCommandQueue->Signal(m_dxFence.Get(), fenceValueForSignal));
	}

	void Direct3D12Renderer::waitForFenceValue(std::chrono::milliseconds duration) {
		if (m_dxFence->GetCompletedValue() < m_dxFenceValue) {
			CHECK_DX(m_dxFence->SetEventOnCompletion(m_dxFenceValue, m_dxFenceEvent));
			::WaitForSingleObject(m_dxFenceEvent, static_cast<DWORD>(duration.count()));
		}
	}

	void Direct3D12Renderer::flush() {
		signalFence();
		waitForFenceValue();
	}

	void Direct3D12Renderer::init() {
#ifdef _DEBUG
		enableDebugLayer();
#endif

		createFactory();
		createAdapter();
		createDevice();
		createCommandQueue();

		checkTearingSupport();
		createSwapChain();
		
		createCommandAllocators();
		createCommandLists();

		createSyncObjects();
		createEventHandle();
	}

	void Direct3D12Renderer::draw() {
		UINT frameIndex = m_dxSwapChain->GetCurrentBackBufferIndex();

		CHECK_DX(m_dxCommandAllocators[frameIndex]->Reset());
		CHECK_DX(m_dxCommandList[frameIndex]->Reset(m_dxCommandAllocators[frameIndex].Get(), nullptr));

		// Define the render target
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_dxRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += frameIndex * m_dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Transition to RENDER_TARGET
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = m_dxBackBuffers[frameIndex].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		m_dxCommandList[frameIndex]->ResourceBarrier(1, &barrier);

		// Clear
		const float clearColor[] = { 129.0f / 255.0f, 186.0f / 255.0f, 219.0f / 255.0f, 1.0f };
		m_dxCommandList[frameIndex]->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		// Transition back to PRESENT
		std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
		m_dxCommandList[frameIndex]->ResourceBarrier(1, &barrier);

		// Close and execute
		CHECK_DX(m_dxCommandList[frameIndex]->Close());
		ID3D12CommandList* cmdLists[] = { m_dxCommandList[frameIndex].Get() };
		m_dxCommandQueue->ExecuteCommandLists(1, cmdLists);

		// Present
		CHECK_DX(m_dxSwapChain->Present(m_dxVsync ? 1 : 0, m_dxTearingSupported ? DXGI_PRESENT_ALLOW_TEARING : 0));

		flush();
	}

	void Direct3D12Renderer::cleanup() {}
}
#endif