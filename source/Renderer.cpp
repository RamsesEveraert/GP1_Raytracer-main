//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"

#include <algorithm>
#include <execution>

#define PARALLEL_EXECUTION

using namespace dae;

Renderer::Renderer(SDL_Window * pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow)),
	m_IsShadowsActive{false},
	m_CurrentLightingMode{LightingMode::Combined}
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

void Renderer::Render(Scene* pScene) const
{
	Camera& camera = pScene->GetCamera();
	const Matrix cameraToWorld = camera.CalculateCameraToWorld();
		
	// window ratio

	const float aspectRatio{ static_cast<float>(m_Width) / static_cast<float>(m_Height) };
	const float fov = camera.fov;
	const float invWidth{ 1.0f / m_Width };
	const float invHeight{ 1.0f / m_Height };

	//Render pixel executions
	
#ifdef PARALLEL_EXECUTION
	// parallel logic
	uint32_t amountOfPixels{ static_cast<uint32_t>(m_Width * m_Height) };
	std::vector<uint32_t>pixelIndices{};

	pixelIndices.reserve(amountOfPixels);

	for (uint32_t index{}; index < amountOfPixels; ++index) pixelIndices.emplace_back(index);

	std::for_each(std::execution::par, pixelIndices.begin(), pixelIndices.end(), [&](int i) 
	{
		RenderPixel(pScene, i, fov, aspectRatio, cameraToWorld, camera.origin);
	});
#else
	// synchronous logic
	uint32_t amountOfPixels{ static_cast<uint32_t>(m_Width * m_Height) };
	for (uint32_t pixelIndex{}; pixelIndex < amountOfPixels; ++pixelIndex)
	{
		RenderPixel(pScene, pixelIndex, fov, aspectRatio, cameraToWorld, camera.origin);
	}

#endif

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}
void dae::Renderer::RenderPixel(Scene* pScene, uint32_t pixelIndex, float fov, float aspectratio, const Matrix& cameraToWorld, const Vector3& cameraOrigin) const
{
	auto materials{ pScene->GetMaterials() };
	auto& lights = pScene->GetLights();

	const uint32_t px{ pixelIndex % m_Width }, py{ pixelIndex / m_Width };

	float rx{ px + 0.5f }, ry{ py + 0.5f };
	float cx{ (2 * (rx / static_cast<float>(m_Width)) - 1) * aspectratio * fov };
	float cy{(1-(2*(ry/static_cast<float>(m_Height)))) * fov};

	Vector3 rayDirection = Vector3(cx, cy, 1).Normalized();

	rayDirection = cameraToWorld.TransformVector(rayDirection);
	Ray viewRay(cameraOrigin, rayDirection);

	ColorRGB finalColor{};

	HitRecord closestHit{};

	// Perform ray-object intersection tests
	pScene->GetClosestHit(viewRay, closestHit);

	// Perform shading calculations
	if (closestHit.didHit)
	{
		for (auto& light : lights)
		{
			Vector3 lightRayDirection = LightUtils::GetDirectionToLight(light, closestHit.origin);

			Ray lightRay;

			lightRay.max = lightRayDirection.Normalize();
			lightRay.origin = closestHit.origin + closestHit.normal * 0.0001f;
			lightRay.direction = lightRayDirection;

			const float lambertCosLaw{ Vector3::Dot(closestHit.normal, lightRayDirection) };

			if (lambertCosLaw < 0) continue;
			if (m_IsShadowsActive && pScene->DoesHit(lightRay))	continue;

			const ColorRGB BRDFrgb{ materials[closestHit.materialIndex]->Shade(closestHit, lightRayDirection, -viewRay.direction) };

			switch (m_CurrentLightingMode)
			{
			case dae::Renderer::LightingMode::ObservedArea:
				finalColor += ColorRGB{ lambertCosLaw,lambertCosLaw,lambertCosLaw };
				break;
			case dae::Renderer::LightingMode::Radiance:
				finalColor += LightUtils::GetRadiance(light, closestHit.origin);
				break;
			case dae::Renderer::LightingMode::BRDF:
				finalColor += BRDFrgb;
				break;
			case dae::Renderer::LightingMode::Combined:
				finalColor += LightUtils::GetRadiance(light, closestHit.origin) * BRDFrgb * lambertCosLaw;
				break;
			default:
				break;
			}

		}
	}

	//Update Color in Buffer
	finalColor.MaxToOne();

	m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
		static_cast<uint8_t>(finalColor.r * 255),
		static_cast<uint8_t>(finalColor.g * 255),
		static_cast<uint8_t>(finalColor.b * 255));

}
bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}

void dae::Renderer::ToggleShadowRendering()
{
	m_IsShadowsActive = !m_IsShadowsActive;
}

void dae::Renderer::CycleLightning()
{
	int cyclePhase{ static_cast<int>(m_CurrentLightingMode) };
	cyclePhase = (cyclePhase + 1) % static_cast<int>(LightingMode::Max); // next + check in interval

	m_CurrentLightingMode = static_cast<LightingMode>(cyclePhase);
}
