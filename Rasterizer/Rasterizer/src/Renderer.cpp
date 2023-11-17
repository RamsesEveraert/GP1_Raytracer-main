//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

namespace dae
{
	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
		m_AspectRatio = static_cast<float>(m_Width) / m_Height;

		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

		m_pDepthBufferPixels = new float[m_Width * m_Height];

		//Initialize Camera
		m_Camera.Initialize(60.f, { 0.0f,0.0f,-10.f });
	}

	Renderer::~Renderer()
	{
		delete[] m_pDepthBufferPixels;
	}

	void Renderer::Update(Timer* pTimer)
	{
		m_Camera.Update(pTimer);
	}

	void Renderer::Render()
	{
		//@START
		std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
		SDL_FillRect(m_pBackBuffer, nullptr, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

		//Lock BackBuffer
		SDL_LockSurface(m_pBackBuffer);

		std::vector<Vertex> vertices{
			{ { 0.0f,  2.0f, 0.0f}, {colors::Red} },
			{ { 1.5f, -1.0f, 0.0f}, {colors::Red} },
			{ {-1.5f, -1.0f, 0.0f}, {colors::Red} },

			{ { 0.0f,  4.0f, 2.0f}, {colors::Red} },
			{ { 3.0f, -2.0f, 2.0f}, {colors::Green} },
			{ {-3.0f, -2.0f, 2.0f}, {colors::Blue} }
		};

		VertexTransformationFunction(vertices, vertices);
		RasterizeTriangles(vertices);

		//@END
		//Update SDL Surface
		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);
	}

	void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
	{
		vertices_out.resize(vertices_in.size());

		for (size_t i = 0; i < vertices_in.size(); ++i)
		{
			vertices_out[i] = vertices_in[i];

			TransformToViewSpace(vertices_out[i].position);
			TransformToProjectionSpace(vertices_out[i].position);
			TransformToScreenSpace(vertices_out[i].position);
		}
	}

	bool Renderer::SaveBufferToImage() const
	{
		return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
	}

	void Renderer::RasterizeTriangles(const std::vector<Vertex>& vertices)
	{
		assert(vertices.size() % 3 == 0 && "Incomplete triangles");

		for (size_t i = 0; i < vertices.size(); i += 3)
		{
			const Vertex& vertex0 = vertices[i];
			const Vertex& vertex1 = vertices[i + 1];
			const Vertex& vertex2 = vertices[i + 2];

			// Calculate data for the triangle
			TriangleData triangleData = CalculateTriangleData(vertex0.position, vertex1.position, vertex2.position);

			// bounding box for the triangle
			float minX = std::min({ vertex0.position.x, vertex1.position.x, vertex2.position.x });
			float minY = std::min({ vertex0.position.y, vertex1.position.y, vertex2.position.y });
			float maxX = std::max({ vertex0.position.x, vertex1.position.x, vertex2.position.x });
			float maxY = std::max({ vertex0.position.y, vertex1.position.y, vertex2.position.y });

			int boxLeft = std::max(0, static_cast<int>(minX));
			int boxTop = std::max(0, static_cast<int>(minY));
			int boxRight = std::min(m_Width, static_cast<int>(maxX));
			int boxBottom = std::min(m_Height, static_cast<int>(maxY));

			for (int px = boxLeft; px <= boxRight; ++px)
			{
				for (int py = boxTop; py <= boxBottom; ++py)
				{
					Vector2 pixel{ px + 0.5f, py + 0.5f };
					float weight0, weight1, weight2;

					if (!IsPixelInTriangle(pixel, vertex0.position, vertex1.position, vertex2.position, weight0, weight1, weight2, triangleData))
						continue;

					float depth{ vertex0.position.z * weight0 + vertex1.position.z * weight1 + vertex2.position.z * weight2 };
					if (!PerformDepthTest(px, py, depth))
						continue;

					ColorRGB color{ vertex0.color * weight0 + vertex1.color * weight1 + vertex2.color * weight2 };
					color.MaxToOne();

					WritePixel(px, py, color, depth);
				}
			}
		}
	}



	bool Renderer::IsPixelInTriangle(const Vector2& point, const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2, float& weight0, float& weight1, float& weight2, const TriangleData& triangleData) const
	{
		Vector2 pointToVertex0 = point - vertex0.GetXY();
		Vector2 pointToVertex1 = point - vertex1.GetXY();
		Vector2 pointToVertex2 = point - vertex2.GetXY();

		// Calculate areas : have to have the same sign 
		float area0 = Vector2::Cross(triangleData.edge1, pointToVertex1);
		if (std::signbit(area0) != std::signbit(triangleData.totalArea))
			return false;

		float area1 = Vector2::Cross(triangleData.edge2, pointToVertex2);
		if (std::signbit(area1) != std::signbit(triangleData.totalArea))
			return false;

		float area2 = Vector2::Cross(triangleData.edge0, pointToVertex0);
		if (std::signbit(area2) != std::signbit(triangleData.totalArea))
			return false;

		weight0 = area0 * triangleData.invTotalArea;
		weight1 = area1 * triangleData.invTotalArea;
		weight2 = area2 * triangleData.invTotalArea;

		return true;
	}



	bool Renderer::PerformDepthTest(int px, int py, float depth) const
	{
		int idx = px + py * m_Width;
		assert(idx < m_Width * m_Height && "index out of bounds");
		return depth < m_pDepthBufferPixels[idx];
	}

	void Renderer::WritePixel(int px, int py, const ColorRGB& color, float depth)
	{
		int idx = px + py * m_Width;
		assert(idx < m_Width * m_Height && "index out of bounds");

		m_pBackBufferPixels[idx] = SDL_MapRGB(
			m_pBackBuffer->format,
			static_cast<uint8_t>(color.r * 255),
			static_cast<uint8_t>(color.g * 255),
			static_cast<uint8_t>(color.b * 255)
		);

		m_pDepthBufferPixels[idx] = depth;
	}

	

	void Renderer::TransformToViewSpace(Vector3& vertex) const
	{
		vertex = m_Camera.viewMatrix.TransformPoint(vertex);
	}

	void Renderer::TransformToProjectionSpace(Vector3& vertex) const
	{
		SetCameraSettings(vertex);
		PerspectiveDivide(vertex);
	}

	void Renderer::TransformToScreenSpace(Vector3& vertex) const
	{
		vertex.x = (vertex.x + 1) * 0.5f * m_Width;
		vertex.y = (1 - vertex.y) * 0.5f * m_Height;
	}

	void Renderer::PerspectiveDivide(Vector3& vertex) const
	{
		vertex.x /= vertex.z;
		vertex.y /= vertex.z;
	}

	void Renderer::SetCameraSettings(Vector3& vertex) const
	{
		vertex.x /= m_AspectRatio * m_Camera.fov;
		vertex.y /= m_Camera.fov;
	}

	Renderer::TriangleData Renderer::CalculateTriangleData(const Vector3& vertex0, const Vector3& vertex1, const Vector3& vertex2)
	{
		TriangleData data;
		data.edge0 = (vertex1 - vertex0).GetXY();
		data.edge1 = (vertex2 - vertex1).GetXY();
		data.edge2 = (vertex0 - vertex2).GetXY();

		data.totalArea = Vector2::Cross(data.edge0, -data.edge2);
		data.invTotalArea = 1 / data.totalArea;

		return data;
	}

}
