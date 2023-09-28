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
	m_pBuffer(SDL_GetWindowSurface(pWindow))
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

	const float FOV{ TO_RADIANS * camera.fovAngle };

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			float gradient = px / static_cast<float>(m_Width);
			gradient += py / static_cast<float>(m_Width);
			gradient /= 2.0f;

			// Calculate ray direction camera
			float x = (2 * (px + 0.5f) * invWidth - 1) * aspectRatio * FOV;
			float y = (1 - 2 * (py + 0.5f) * invHeight) * FOV;
			Vector3 rayDirection = Vector3(x, y, 1).Normalized();
			
			//Ray viewRay(Vector3{}, rayDirection);
			Ray viewRay(camera.origin, rayDirection);

			ColorRGB finalColor{ rayDirection.x, rayDirection.y, rayDirection.z };

			HitRecord closestHit{};

			// Perform ray-object intersection tests
			pScene->GetClosestHit(viewRay, closestHit);

			if (closestHit.didHit)
			{
				// Perform shading calculations
				const float scaled_t = (closestHit.t - 50.f) / 40.f;
				finalColor = materials[closestHit.materialIndex]->Shade();
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
