#ifdef NASHI_USE_DIRECT3D12
#include <renderer_d3d12.hpp>

namespace Nashi {
	Direct3D12Renderer::Direct3D12Renderer(SDL_Window* window, SDL_Event event, HWND hwnd) {
		this->m_window = window;
		this->m_event = event;
		this->m_hwnd = hwnd;
	}
#ifdef _DEBUG
	void Direct3D12Renderer::enableDebugLayer() {
		ComPtr<ID3D12Debug> debugInterface;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
			debugInterface->EnableDebugLayer();
			OutputDebugStringA("D3D12 Debug Layer ENABLED\n");
		}
		else {
			OutputDebugStringA("WARNING: Could not enable D3D12 Debug Layer!\n");
		}
		
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
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

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

			CHECK_DX(infoQueue->AddStorageFilterEntries(&newFilter));
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
		SDL_GetWindowSizeInPixels(m_window, &m_windowWidth, &m_windowHeight);
		
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
		swapChainDesc.Width = m_windowWidth;
		swapChainDesc.Height = m_windowHeight;
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

		m_dxCurrentBackBufferIndex = m_dxSwapChain->GetCurrentBackBufferIndex();

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
			CHECK_DX(m_dxDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_dxCommandAllocators[i].Get(), nullptr, IID_PPV_ARGS(&m_dxCommandLists[i])));
			
			CHECK_DX(m_dxCommandLists[i]->Close());
		}

	}

	void Direct3D12Renderer::createSyncObjects() {
		CHECK_DX(m_dxDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_dxFence)));
	}

	void Direct3D12Renderer::createEventHandle() {
		m_dxFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		assert(m_dxFenceEvent && "Failed to create fence event");
	}

	uint64_t Direct3D12Renderer::signalFence() {
		uint64_t fenceValueForSignal = ++m_dxFenceValue;
		CHECK_DX(m_dxCommandQueue->Signal(m_dxFence.Get(), fenceValueForSignal));

		return fenceValueForSignal;
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

	void Direct3D12Renderer::resizeWindow() {
		flush();

		for (int i = 0; i < m_dxNumFrames; ++i) {
			m_dxBackBuffers[i].Reset();
		}

		m_dxRTVDescriptorHeap.Reset();

		SDL_GetWindowSizeInPixels(m_window, &m_windowWidth, &m_windowHeight);

		while (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED) {
			SDL_WaitEvent(&m_event);
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		CHECK_DX(m_dxSwapChain->GetDesc(&swapChainDesc));

		CHECK_DX(m_dxSwapChain->ResizeBuffers(
			m_dxNumFrames,
			m_windowWidth,
			m_windowHeight,
			swapChainDesc.BufferDesc.Format,
			swapChainDesc.Flags
		));

		m_dxCurrentBackBufferIndex = m_dxSwapChain->GetCurrentBackBufferIndex();

		createDescriptorHeap();
		updateRenderTargetViews();
		createViewport();
	}

	void Direct3D12Renderer::createStencilBuffer() {
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		
		CHECK_DX(m_dxDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dxDepthStencilBufferHeap)));

		D3D12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_D32_FLOAT, m_windowWidth, m_windowHeight, 1, 1
		);
		depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE depthClearValue{};
		depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthClearValue.DepthStencil = { 1.0f, 0 };

		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CHECK_DX(m_dxDevice->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue,
			IID_PPV_ARGS(&m_dxDepthStencilBuffer)
		));

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		m_dxDevice->CreateDepthStencilView(m_dxDepthStencilBuffer.Get(), &dsvDesc,
			m_dxDepthStencilBufferHeap->GetCPUDescriptorHandleForHeapStart());
	}

	void Direct3D12Renderer::createVertexBuffer()
	{
		ComPtr<ID3D12Resource> vertexBufferUpload;
		size_t bufferSize = sizeof(m_vertices[0]) * sizeof(Vertex);

		// Create default heap (GPU local)
		const CD3DX12_HEAP_PROPERTIES defaultHeapProps{ D3D12_HEAP_TYPE_DEFAULT };
		const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

		CHECK_DX(m_dxDevice->CreateCommittedResource(
			&defaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&m_dxVertexBuffer)));

		// Create upload heap
		const CD3DX12_HEAP_PROPERTIES uploadHeapProps{ D3D12_HEAP_TYPE_UPLOAD };
		CHECK_DX(m_dxDevice->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBufferUpload)));

		// Map and copy vertex data to upload heap
		Vertex* data = nullptr;
		CHECK_DX(vertexBufferUpload->Map(0, nullptr, reinterpret_cast<void**>(&data)));
		std::ranges::copy(m_vertices, data);
		vertexBufferUpload->Unmap(0, nullptr);

		// Reset command allocator and command list for current back buffer
		CHECK_DX(m_dxCommandAllocators[m_dxCurrentBackBufferIndex]->Reset());
		CHECK_DX(m_dxCommandLists[m_dxCurrentBackBufferIndex]->Reset(
			m_dxCommandAllocators[m_dxCurrentBackBufferIndex].Get(),
			nullptr));

		// Transition from COMMON to COPY_DEST before copy
		const auto barrierBeforeCopy = CD3DX12_RESOURCE_BARRIER::Transition(
			m_dxVertexBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST);
		m_dxCommandLists[m_dxCurrentBackBufferIndex]->ResourceBarrier(1, &barrierBeforeCopy);

		// Copy data from upload heap to default heap
		m_dxCommandLists[m_dxCurrentBackBufferIndex]->CopyResource(m_dxVertexBuffer.Get(), vertexBufferUpload.Get());

		// Transition vertex buffer to VERTEX_AND_CONSTANT_BUFFER for use in IA stage
		const auto barrierAfterCopy = CD3DX12_RESOURCE_BARRIER::Transition(
			m_dxVertexBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		m_dxCommandLists[m_dxCurrentBackBufferIndex]->ResourceBarrier(1, &barrierAfterCopy);

		CHECK_DX(m_dxCommandLists[m_dxCurrentBackBufferIndex]->Close());

		// Execute command list to perform copy and transition
		ID3D12CommandList* const commandLists[] = { m_dxCommandLists[m_dxCurrentBackBufferIndex].Get() };
		m_dxCommandQueue->ExecuteCommandLists((UINT)std::size(commandLists), commandLists);

		// Wait for GPU to finish copying
		m_dxFrameFenceValues[m_dxCurrentBackBufferIndex] = signalFence();
		waitForFenceValue();

		// Setup vertex buffer view for IA stage
		m_dxVertexBufferView.BufferLocation = m_dxVertexBuffer->GetGPUVirtualAddress();
		m_dxVertexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
		m_dxVertexBufferView.StrideInBytes = sizeof(Vertex);
	}

	void Direct3D12Renderer::createIndexBuffer() {
		const UINT indexBufferSize = static_cast<UINT>(m_indices.size() * sizeof(uint16_t));

		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

		CHECK_DX(m_dxDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_dxIndexBuffer)
		));

		uint16_t* indexData = nullptr;
		CD3DX12_RANGE readRange{ 0, 0 };

		CHECK_DX(m_dxIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&indexData)));
		memcpy(indexData, m_indices.data(), indexBufferSize);
		m_dxIndexBuffer->Unmap(0, nullptr);

		m_dxIndexBufferView.BufferLocation = m_dxIndexBuffer->GetGPUVirtualAddress();
		m_dxIndexBufferView.SizeInBytes = indexBufferSize;
		m_dxIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	}

	void Direct3D12Renderer::createConstantBuffer() {
		const UINT constantBufferSize = (sizeof(UniformBufferObject) + 255) & ~255;

		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

		CHECK_DX(m_dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
			&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, 
			nullptr, IID_PPV_ARGS(&m_dxConstantBuffer)));

		CD3DX12_RANGE readRange{ 0, 0 };

		CHECK_DX(m_dxConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_dxUbo)));

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		heapDesc.NumDescriptors = 1;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		CHECK_DX(m_dxDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_dxConstantBufferHeap)));

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
		cbvDesc.BufferLocation = m_dxConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = constantBufferSize;

		m_dxDevice->CreateConstantBufferView(&cbvDesc, m_dxConstantBufferHeap->GetCPUDescriptorHandleForHeapStart());
	}

	void Direct3D12Renderer::updateUniformBufferObject() {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		const auto modelMatrix = XMMatrixTranspose(XMMatrixRotationZ(time * XMConvertToRadians(45.0f)));

		const auto eyePosition = XMVectorSet(2.0f, 2.0f, 2.0f, 1.0f);
		const auto focusPosition = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		const auto upDirection = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		const auto viewMatrix = XMMatrixLookAtLH(eyePosition, focusPosition, upDirection);

		const auto aspectRatio = float(m_windowWidth) / float(m_windowHeight);
		const auto projMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), aspectRatio, 0.1f, 10.0f);

		m_dxUbo->model = modelMatrix;
		m_dxUbo->view = viewMatrix;
		m_dxUbo->proj = projMatrix;
	}

	void Direct3D12Renderer::createRootSignature() {
		CD3DX12_DESCRIPTOR_RANGE cbvRange;
		cbvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

		CD3DX12_ROOT_PARAMETER rootParameters[1];
		rootParameters[0].InitAsDescriptorTable(1, &cbvRange, D3D12_SHADER_VISIBILITY_VERTEX);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init((UINT)std::size(rootParameters), rootParameters,
			0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signatureBlob;
		ComPtr<ID3DBlob> errorBlob;
		if (const auto hr = D3D12SerializeRootSignature(
			&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
			&signatureBlob, &errorBlob
		); FAILED(hr)) {
			if (errorBlob) {
				throw std::runtime_error("Creating root signature went wrong");
			}
			CHECK_DX(hr);
		}
		CHECK_DX(m_dxDevice->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_dxRootSignature)));

	}

	void Direct3D12Renderer::createGraphicsPipeline() {
		const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		};

		ComPtr<ID3DBlob> vertexShaderBlob;
		CHECK_DX(D3DReadFileToBlob(L"shaders/basic.vert.cso", &vertexShaderBlob));

		ComPtr<ID3DBlob> pixelShaderBlob;
		CHECK_DX(D3DReadFileToBlob(L"shaders/basic.frag.cso", &pixelShaderBlob));

		m_dxPipelineStateStream.RootSignature = m_dxRootSignature.Get();
		m_dxPipelineStateStream.InputLayout = { inputLayout, (UINT)std::size(inputLayout) };
		m_dxPipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		m_dxPipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		m_dxPipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		m_dxPipelineStateStream.RTVFormats = {
			.RTFormats{ DXGI_FORMAT_B8G8R8A8_UNORM  },
			.NumRenderTargets = 1,
		};
		
		CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthStencilDesc.StencilEnable = FALSE;

		m_dxPipelineStateStream.DepthStencilState = depthStencilDesc;
		m_dxPipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;

		CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FrontCounterClockwise = FALSE;
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

		m_dxPipelineStateStream.RasterizerState = rasterizerDesc;

		const D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
			sizeof(PipelineStateStream), &m_dxPipelineStateStream
		};

		CHECK_DX(m_dxDevice->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_dxPipelineState)));
	}

	void Direct3D12Renderer::createViewport() {
		const CD3DX12_RECT scissorRect{ 0, 0, LONG_MAX, LONG_MAX };
		m_dxScissorRect = scissorRect;


		const CD3DX12_VIEWPORT viewport{ 0.0f, 0.0f, float(m_windowWidth), float(m_windowHeight) };
		m_dxViewport = viewport;
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

		createStencilBuffer();
		createVertexBuffer();
		createIndexBuffer();
		createConstantBuffer();

		createRootSignature();
		createGraphicsPipeline();

		createViewport();
	}

	void Direct3D12Renderer::draw() {
		if (m_windowResized) {
			resizeWindow();
			m_windowResized = false;
		}

		m_dxCurrentBackBufferIndex = m_dxSwapChain->GetCurrentBackBufferIndex();

		auto commandAllocator = m_dxCommandAllocators[m_dxCurrentBackBufferIndex];
		auto backBuffer = m_dxBackBuffers[m_dxCurrentBackBufferIndex];
		auto commandList = m_dxCommandLists[m_dxCurrentBackBufferIndex];
		commandAllocator->Reset();
		commandList->Reset(commandAllocator.Get(), nullptr);


		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				backBuffer.Get(),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			commandList->ResourceBarrier(1, &barrier);

		}
		
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_dxRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		UINT rtvDescriptorSize = m_dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		rtvHandle.ptr += m_dxCurrentBackBufferIndex * rtvDescriptorSize;

		FLOAT clearColor[] = { 129.0f / 255.0f, 186.0f / 255.0f, 219.0f / 255.0f, 1.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		commandList->SetPipelineState(m_dxPipelineState.Get());
		commandList->SetGraphicsRootSignature(m_dxRootSignature.Get());

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &m_dxVertexBufferView);
		commandList->IASetIndexBuffer(&m_dxIndexBufferView);

		commandList->RSSetScissorRects(1, &m_dxScissorRect);
		commandList->RSSetViewports(1, &m_dxViewport);

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dxDepthStencilBufferHeap->GetCPUDescriptorHandleForHeapStart();
		commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		ID3D12DescriptorHeap* descriptorHeaps[] = { m_dxConstantBufferHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
		updateUniformBufferObject();

		commandList->SetGraphicsRootDescriptorTable(0, m_dxConstantBufferHeap->GetGPUDescriptorHandleForHeapStart());

		commandList->DrawIndexedInstanced((UINT)m_indices.size(), 1, 0, 0, 0);

		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				backBuffer.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
			commandList->ResourceBarrier(1, &barrier);
		}

		CHECK_DX(commandList->Close());

		ID3D12CommandList* const commandLists[] = {
			commandList.Get()
		};

		m_dxCommandQueue->ExecuteCommandLists(std::size(commandLists), commandLists);

		UINT syncInternal = m_dxVsync ? 1 : 0;
		UINT presentFlags = m_dxTearingSupported && !m_dxVsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
		CHECK_DX(m_dxSwapChain->Present(syncInternal, presentFlags));

		m_dxFrameFenceValues[m_dxCurrentBackBufferIndex] = signalFence();
		waitForFenceValue();

	}

	void Direct3D12Renderer::cleanup() {
		flush();
	}
}
#endif