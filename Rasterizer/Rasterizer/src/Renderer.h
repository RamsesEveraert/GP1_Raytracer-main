#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"
#include "ColorRGB.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	struct Vertex_Out;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;
		void ToggleDebugDepthBuffer();
		void ToggleDebugRotation();
		void ToggleNormalMapping();

		void VertexTransformationFunction(Mesh& mesh) const;

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		Vector3 m_GlobalLightDirection{ 0.577f, -0.577f, 0.577f };
		float m_Shininess{ 25.0f };

		// mesh debug 
		Mesh m_Mesh{};
		Texture* m_pAlbedoTexture{ nullptr };
		Texture* m_pNormalTexture{ nullptr };
		Texture* m_pGlossTexture{ nullptr };
		Texture* m_pSpecularTexture{ nullptr };
		bool m_IsMeshRotating{ true };
		// ====================

		int m_Width{};
		int m_Height{};
		float m_AspectRatio{};

		bool m_DebugDepthBuffer{};
		bool m_NormalMapping{ false };

	private:

		void RenderMesh();

		void RasterizeTriangleStrip(const Mesh& mesh);
		void RasterizeTriangleList(const Mesh& mesh);
		void RasterizeTriangle(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2);

		void PerspectiveDivide(Vertex_Out& vertex) const;
		void TransformToScreenSpace(Vertex_Out& vertex) const;

		void ShadePixel(int pixelIndex, const Vertex_Out& v) const;
	};
}
