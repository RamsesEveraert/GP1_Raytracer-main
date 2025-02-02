#include "pch.h"
#include "Renderer.h"
#include "Utils.h"




namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//Create and Initialize Camera
		m_pCamera = std::make_unique<Camera>(Vector3::Zero, 45.0f);
		float aspectRatio = static_cast<float>(m_Width) / m_Height;
		//TODO: nearplae far plane
		m_pCamera->Initialize(aspectRatio, 45.0f, Vector3{0.f,0.f,-50.f}, 0.01f, 1000.0f);
		m_pCamera->moveSpeed = 30.0f;

		//Initialize DirectX pipeline
		const HRESULT result = InitializeDirectX();
		if (result == S_OK)
		{
			m_IsInitialized = true;
			std::cout << "DirectX is initialized and ready!\n";
		}
		else
		{
			std::cout << "DirectX initialization failed!\n";
		}

		//Create some data for our mesh

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		Utils::ParseOBJ("Resources/vehicle.obj", vertices, indices);

		m_pMesh = std::make_unique<Mesh>(m_pDevice, vertices, indices);
		m_pMesh->SetWorldMatrix(Matrix::CreateTranslation(0.0f, 0.0f, 5.0f));
		m_pMesh->SetDiffuseMap("Resources/vehicle_diffuse.png");
		m_pMesh->SetNormalMap("Resources/vehicle_normal.png");
		m_pMesh->SetSpecularMap("Resources/vehicle_specular.png");
		m_pMesh->SetGlossinessMap("Resources/vehicle_gloss.png");
	}

	Renderer::~Renderer()
	{
		// reversed order as created!
		if (m_pRenderTargetView) m_pRenderTargetView->Release();
		if (m_pRenderTargetBuffer) m_pRenderTargetBuffer->Release();
		if (m_pDepthStencilView) m_pDepthStencilView->Release();
		if (m_pDepthStencilBuffer) m_pDepthStencilBuffer->Release();
		if (m_pSwapChain) m_pSwapChain->Release();

		if (m_pDeviceContext)
		{
			m_pDeviceContext->ClearState();
			m_pDeviceContext->Flush();
			m_pDeviceContext->Release();
		}

		if (m_pDevice) m_pDevice->Release();

	}

	void Renderer::Update(const Timer* pTimer)
	{
		// Update camera
		m_pCamera->Update(pTimer);

		//Update rotation
		if (m_IsRotating) m_pMesh->SetWorldMatrix(Matrix::CreateRotationY(pTimer->GetElapsed()) * m_pMesh->GetWorldMatrix());
	}


	void Renderer::Render() const
	{
		if (!m_IsInitialized)
			return;

		// 1. Clear RTV & DSV

		constexpr float color[4] = { 0.39f,0.59f,0.93f,1.f };
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, color);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH || D3D11_CLEAR_STENCIL, 1, 0);

		// 2. Set Pipeline + Invoke Draw Calls (= RENDER)

		m_pMesh->Render(m_pCamera.get(), m_pDeviceContext);

		// 3. Present Backbuffer (Swap)
		m_pSwapChain->Present(0, 0);

	}

	void Renderer::ToggleRotation()
	{
		m_IsRotating = !m_IsRotating;
	}

	HRESULT Renderer::InitializeDirectX()
	{
		// 1. Create device and device context

		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
		uint32_t createDeviceFlags = 0;

		#if defined(DEBUG) || defined (_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		#endif	

		HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel, 1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);

		if (FAILED(result))
		{
			return result;
		}

		// 2. Create DxGI factory

		IDXGIFactory1* pDxGIFactory{nullptr};

		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1),  // compiler-specific extension to get the UUID (Universally Unique Identifier) of the IDXGIFactory1 interface
			reinterpret_cast<void**>(&pDxGIFactory));

		if (FAILED(result))
		{
			return result;
		}

		// 3. Create SwapChain

		// Get handle (HWND) from the SDL backbuffer
		SDL_SysWMinfo sysWMinfo{};
		SDL_GetVersion(&sysWMinfo.version);
		SDL_GetWindowWMInfo(m_pWindow, &sysWMinfo);

		// Initialize swapchain
		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;
		swapChainDesc.OutputWindow = sysWMinfo.info.win.window;

		// Create swapchain
		result = pDxGIFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);

		if (FAILED(result))
		{
			return result;
		}

		// 4. Create DepthStencil (DS) & DepthStencilView (DSV) : depth buffer to solve visibility problems
		
		// DepthStencil : mask pixels in image, controls pixel is draw before Z-test
		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);

		if (FAILED(result))
		{
			return result;
		}

		//DepthSTencilView
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer,&depthStencilViewDesc,&m_pDepthStencilView);

		if (FAILED(result))
		{
			return result;
		}

		// 5. Create RenderTarget (RT) & RenderTargetView (RTV)
		
		// RenderTarget = RESOURCE: creates resources for drawing + performs drawing operations -> resource: shared by multiple pipeline stages
		// retrieve back buffer resource from swapchain and create resource view using this pointer
		result = m_pSwapChain->GetBuffer(0,__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));

		if (FAILED(result))
		{
			return result;
		}

		// RenderTargetView = VIEW: how a resource is used inthe pipeline

		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);

		if (FAILED(result))
		{
			return result;
		}

		// 6. Bind RTV & DSV to output merger state: active buffers during output merger stage
		
		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		// 7. Set ViewPort: where content of backbuffer will be rendered on the screen (used to transform NDC to screenspace)

		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(m_Width);
		viewport.Height = static_cast<float>(m_Height);
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		m_pDeviceContext->RSSetViewports(1, &viewport);

		pDxGIFactory->Release();

		return S_OK;
	}
}
