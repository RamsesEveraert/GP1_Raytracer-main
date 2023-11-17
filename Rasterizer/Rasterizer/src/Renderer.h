#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include "Camera.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
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

		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const;

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};
		float m_AspectRatio{};

	private:

		struct TriangleData 
		{
			Vector2 edge0, edge1, edge2;
			float totalArea;
			float invTotalArea;
		};

		void TransformToViewSpace(Vector3& vertex) const;
		void TransformToProjectionSpace(Vector3& vertex) const;
		void TransformToScreenSpace(Vector3& vertex) const;

		void PerspectiveDivide(Vector3& vertex) const;
		void SetCameraSettings(Vector3& vertex) const;

		void RasterizeTriangles(const std::vector<Vertex>& vertices);
		bool IsPixelInTriangle(const Vector2& point, const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2, float& weight0, float& weight1, float& weight2, const TriangleData& triangleData) const;
		bool PerformDepthTest(int px, int py, float depth) const;
		void WritePixel(int px, int py, const ColorRGB& color, float depth);

		TriangleData CalculateTriangleData(const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2);

	};
}
