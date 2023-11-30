#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		SDL_Surface* pSurface = IMG_Load(path.c_str());
		return  new Texture(pSurface);
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) return colors::Black;

		auto px = static_cast<Uint32>(uv.x * m_pSurface->w);
		auto py = static_cast<Uint32>(uv.y * m_pSurface->h);

		Uint8 r, g, b;
		SDL_GetRGB(m_pSurfacePixels[px + py * m_pSurface->w], m_pSurface->format, &r, &g, &b);

		return {
			r / 255.0f,
			g / 255.0f,
			b / 255.0f
		};
	}

	Vector3 Texture::SampleNormal(const Vector2& uv) const
	{
		ColorRGB sample = Sample(uv);

		return {
			2 * sample.r - 1,
			2 * sample.g - 1,
			2 * sample.b - 1
		};
	}

	float Texture::SampleGray(const Vector2& uv) const
	{
		ColorRGB sample = Sample(uv);
		return sample.r;
	}

}