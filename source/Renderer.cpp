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
	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();

	// window ratio

	const float aspectRatio{ static_cast<float>(m_Width) / static_cast<float>(m_Height) };
	const float invWidth{ 1.0f / m_Width };
	const float invHeight{ 1.0f / m_Height };

	// camera

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			// Calculate ray direction camera
			float cx = (2 * (px + 0.5f) * invWidth - 1) * aspectRatio * camera.fov;
			float cy = (1 - 2 * (py + 0.5f) * invHeight) * camera.fov;
			Vector3 rayDirection = Vector3(cx, cy, 1).Normalized();
			
			rayDirection = camera.cameraToWorld.TransformVector(rayDirection);
			Ray viewRay(camera.origin, rayDirection);

			ColorRGB finalColor{};

			HitRecord closestHit{};

			// Perform ray-object intersection tests
			pScene->GetClosestHit(viewRay, closestHit);

			// Perform shading calculations




			if (closestHit.didHit)
			{
				for (auto& light : lights)
				{
					Vector3 lightRayDirection{ LightUtils::GetDirectionToLight(light, closestHit.origin)};
					const Vector3 lightRayOrigin{ closestHit.origin + closestHit.normal * 0.0001f };

					const float lightRayLength{ lightRayDirection.Normalize() }; // normalized na assignment

					Ray lightRay{ lightRayOrigin, lightRayDirection};
					lightRay.max = lightRayLength;

					const float lambertCosLaw{ Vector3::Dot(closestHit.normal, lightRayDirection) };

					if (lambertCosLaw < 0) continue;
					if (m_IsShadowsActive && pScene->DoesHit(lightRay))	continue;

					const ColorRGB BRDFrgb{ materials[closestHit.materialIndex]->Shade(closestHit, lightRay.direction, viewRay.direction) };

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
	}

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
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
