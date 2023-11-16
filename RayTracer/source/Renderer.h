#pragma once

#include <cstdint>
#include "DataTypes.h"
#include "Material.h"

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

		void RenderPixel(Scene* pScene, uint32_t pixelIndex, float fov, float aspectratio, const Matrix& cameraToWorld, const Vector3& cameraOrigin) const;

		void CalculatePixelCoordinates(uint32_t pixelIndex, float fov, float aspectratio, const Matrix& cameraToWorld, uint32_t& px, uint32_t& py, Vector3& rayDirection) const;
		dae::ColorRGB CalculateColor(Scene* pScene, const Ray& viewRay, const std::vector<Material*>& materials, const std::vector<Light>& lights) const;

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
		float m_InvWidth{};
		float m_InvHeight{};

		float m_AspectRatio{};

		bool m_IsShadowsActive;
	};
}
