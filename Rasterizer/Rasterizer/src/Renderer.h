#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include "Camera.h"
#include "DataTypes.h"

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

		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out) const;

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		Texture* m_pTestTexture;

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};
		float m_AspectRatio{};

	private:

		// Weekly assignments

		void AssignmentWeek01();
		void AssignmentWeek02();

		// Functions

		void RasterizeTriangleStrip(const Mesh& mesh);
		void RasterizeTriangleList(const Mesh& mesh);

		void TransformToViewSpace(Vector4& vertex) const;
		void TransformToProjectionSpace(Vector4& vertex) const;
		void TransformToScreenSpace(Vector4& vertex) const;

		void PerspectiveDivide(Vector4& vertex) const;
		void SetCameraSettings(Vector4& vertex) const;

		
		void RasterizeTriangle(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2);

		bool IsPixelInTriangle(const Vector2& point, const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2, float& weight0, float& weight1, float& weight2, const Triangle& triangle) const;
		bool PerformDepthTest(int px, int py, float depth) const;
		void WritePixel(int px, int py, const ColorRGB& color, float depth);



		Triangle CalculateTriangleData(const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2);

	};
}
