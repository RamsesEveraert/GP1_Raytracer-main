#pragma once
#include <SDL_surface.h>
#include <string>
#include <memory>
#include "ColorRGB.h"
#include "Vector3.h"

namespace dae
{
	struct Vector2;

	class Texture
	{
	public:
		Texture(ID3D11Device* pDevice, const std::string& filepath);
		~Texture();

		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;
		Texture(Texture&&) = delete;
		Texture& operator=(Texture&&);

		ID3D11ShaderResourceView* GetShaderResourceView() const;

	private:
		ID3D11Texture2D* m_pBuffer{nullptr};
		ID3D11ShaderResourceView* m_pShaderResourceView{ nullptr };
	};
}