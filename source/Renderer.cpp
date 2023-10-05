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
	m_IsShadowsActive{false}
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
				finalColor = materials[closestHit.materialIndex]->Shade();
				for (auto& light : lights)
				{
					const Vector3 lightRayDirection{ light.origin - closestHit.origin };
					const Vector3 lightRayOrigin{ closestHit.origin + closestHit.normal * 0.0001f };

					Ray lightRay{ lightRayOrigin, lightRayDirection.Normalized() };
					lightRay.max = lightRayDirection.Magnitude();

					if (pScene->DoesHit(lightRay) && m_IsShadowsActive)
					{
						finalColor *= 0.5f;
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
