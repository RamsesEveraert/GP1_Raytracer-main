#include "pch.h"
#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(ID3D11Device* pDevice, const std::string& filepath)
	{
		SDL_Surface* pSurface = IMG_Load(filepath.data());

		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = pSurface->w;
		desc.Height = pSurface->h;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData{};
		initData.pSysMem = pSurface->pixels;
		initData.SysMemPitch = static_cast<UINT>(pSurface->pitch);
		initData.SysMemSlicePitch = static_cast<UINT>(pSurface->h * pSurface->pitch);

		HRESULT result = pDevice->CreateTexture2D(&desc, &initData, &m_pBuffer);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		result = pDevice->CreateShaderResourceView(m_pBuffer, &srvDesc, &m_pShaderResourceView);

		SDL_FreeSurface(pSurface);
	}

	Texture::~Texture()
	{
		if (m_pShaderResourceView) m_pShaderResourceView->Release();
		if (m_pBuffer) m_pBuffer->Release();
	}

	ID3D11ShaderResourceView* Texture::GetShaderResourceView() const
	{
		return m_pShaderResourceView;
	}

}