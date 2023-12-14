#pragma once

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Mesh;
	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void Render() const;

	private:
		SDL_Window* m_pWindow{nullptr};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };

		Mesh* m_pMesh;

		// DirectX
		ID3D11Device* m_pDevice{};
		ID3D11DeviceContext* m_pDeviceContext{};

		IDXGISwapChain* m_pSwapChain{nullptr};

		ID3D11Texture2D* m_pDepthStencilBuffer{nullptr};
		ID3D11DepthStencilView* m_pDepthStencilView{nullptr};

		ID3D11Texture2D* m_pRenderTargetBuffer{nullptr};
		ID3D11RenderTargetView* m_pRenderTargetView{nullptr};

	private:

		//DIRECTX
		HRESULT InitializeDirectX();
		//...
	};
}
