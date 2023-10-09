#pragma once

#include <cstdint>

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer() = default;

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Render(Scene* pScene) const;
		bool SaveBufferToImage() const;

		void ToggleShadowRendering();
		void CycleLightning();

	private:

		enum class LightingMode
		{
			ObservedArea, // Lambert Cosine Law
			Radiance, // Incident Radiance
			BRDF, // Scattering of light
			Combined, // ObservedArea * Radiance * BRDF
			Max
		};

		LightingMode m_CurrentLightingMode;

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pBuffer{};
		uint32_t* m_pBufferPixels{};

		int m_Width{};
		int m_Height{};
		bool m_IsShadowsActive;
	};
}
