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

		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

		m_pDepthBufferPixels = new float[m_Width * m_Height];

		// texture
		m_pTestTexture = Texture::LoadFromFile("Resources/uv_grid_2.png");

		//Initialize Camera
		m_Camera.Initialize(60.f, { 0.0f,0.0f,-10.f });
		m_Camera.aspectRatio = static_cast<float>(m_Width) / m_Height;
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

		//assignment 1
		//AssignmentWeek01();
		AssignmentWeek02();

		//@END
		//Update SDL Surface
		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);
	}

#pragma region Assignments

	void Renderer::AssignmentWeek01()
	{
	/*	std::vector<Vertex> vertices{
			{ { 0.0f,  2.0f, 0.0f}, {colors::Red} },
			{ { 1.5f, -1.0f, 0.0f}, {colors::Red} },
			{ {-1.5f, -1.0f, 0.0f}, {colors::Red} },

			{ { 0.0f,  4.0f, 2.0f}, {colors::Red} },
			{ { 3.0f, -2.0f, 2.0f}, {colors::Green} },
			{ {-3.0f, -2.0f, 2.0f}, {colors::Blue} }
		};

		VertexTransformationFunction(vertices, vertices);
		RasterizeTriangles(vertices);*/
	}

	void Renderer::AssignmentWeek02()
	{
		//std::vector<Mesh> meshes
		//{
		//	Mesh
		//	{
		//		//vertices
		//		{
		//			Vertex{{-3,3,-2}},
		//			Vertex{{0,3,-2}},
		//			Vertex{{3,3,-2}},
		//			Vertex{{-3,0,-2}},
		//			Vertex{{0,0,-2}},
		//			Vertex{{3,0,-2}},
		//			Vertex{{-3,-3,-2}},
		//			Vertex{{0,-3,-2}},
		//			Vertex{{3,-3,-2}},
		//		},
		//			//indices
		//			{
		//				3,0,4,1,5,2,
		//				2,6,
		//				6,3,7,4,8,5
		//			},

		//		PrimitiveTopology::TriangleStrip
		//	}
		//};

		std::vector<Mesh> meshes
		{
			Mesh
			{
				//vertices
				{
					Vertex{ {-3,  3, -2}, colors::White, {0.0f, 0.0f} },
					Vertex{ { 0,  3, -2}, colors::White, {0.5f, 0.0f} },
					Vertex{ { 3,  3, -2}, colors::White, {1.0f, 0.0f} },
					Vertex{ {-3,  0, -2}, colors::White, {0.0f, 0.5f} },
					Vertex{ { 0,  0, -2}, colors::White, {0.5f, 0.5f} },
					Vertex{ { 3,  0, -2}, colors::White, {1.0f, 0.5f} },
					Vertex{ {-3, -3, -2}, colors::White, {0.0f, 1.0f} },
					Vertex{ { 0, -3, -2}, colors::White, {0.5f, 1.0f} },
					Vertex{ { 3, -3, -2}, colors::White, {1.0f, 1.0f} },
				},
				//indices
				{
					3, 0, 1,	1, 4, 3,	4, 1, 2,
					2, 5, 4,	6, 3, 4,	4, 7, 6,
					7, 4, 5,	5, 8, 7,
				},

				PrimitiveTopology::TriangleList
			}
		};

		for (auto& mesh : meshes)
		{
			VertexTransformationFunction(mesh.vertices, mesh.vertices_out);

			switch (mesh.primitiveTopology)
			{
			case PrimitiveTopology::TriangleList:
				RasterizeTriangleList(mesh);
				break;

			case PrimitiveTopology::TriangleStrip:
				RasterizeTriangleStrip(mesh);
				break;
			}
		}

	}
#pragma endregion


	void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out) const
	{
		vertices_out.resize(vertices_in.size());

		Matrix worldMatrix{ Matrix::Identity() };


		// Calculate WorldViewProjection matrix
		Matrix worldViewProjectionMatrix{ worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };

		for (size_t i = 0; i < vertices_in.size(); ++i)
		{
			// Copy over the vertex data
			vertices_out[i].color = vertices_in[i].color;
			vertices_out[i].position = Vector4{ vertices_in[i].position, 1.f }; 
			vertices_out[i].uv = vertices_in[i].uv;

			// Apply the WorldViewProjection transformation
			vertices_out[i].position = worldViewProjectionMatrix.TransformPoint(vertices_out[i].position);
			
			PerspectiveDivide(vertices_out[i]);
			TransformToScreenSpace(vertices_out[i]);

		}
	}

	bool Renderer::SaveBufferToImage() const
	{
		return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
	}


	void Renderer::RasterizeTriangleList(const Mesh& mesh)
	{
		assert(mesh.indices.size() % 3 == 0 && "incomplete triangles");

		for (size_t i = 0; i < mesh.indices.size(); i += 3)
		{
			const Vertex_Out& v0 = mesh.vertices_out[mesh.indices[i]];
			const Vertex_Out& v1 = mesh.vertices_out[mesh.indices[i + 1]];
			const Vertex_Out& v2 = mesh.vertices_out[mesh.indices[i + 2]];

			RasterizeTriangle(v0, v1, v2);
		}
	}

	void Renderer::RasterizeTriangleStrip(const Mesh& mesh)
	{
		for (size_t i = 2; i < mesh.indices.size(); ++i)
		{
			size_t i0 = i - ((i % 2) == 0 ? 2 : 1);
			size_t i1 = i - ((i % 2) == 0 ? 1 : 2);
			size_t i2 = i;

			const Vertex_Out& v0 = mesh.vertices_out[mesh.indices[i0]];
			const Vertex_Out& v1 = mesh.vertices_out[mesh.indices[i1]];
			const Vertex_Out& v2 = mesh.vertices_out[mesh.indices[i2]];

			RasterizeTriangle(v0, v1, v2);
		}
	}

	void Renderer::RasterizeTriangle(const Vertex_Out& vertex0, const Vertex_Out& vertex1, const Vertex_Out& vertex2)
	{
		// Calculate data for the triangle
		Triangle triangle = CalculateTriangleData(vertex0, vertex1, vertex2);

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

					if (!IsPixelInTriangle(pixel, vertex0, vertex1, vertex2, weight0, weight1, weight2, triangle))
						continue;

					float zBufferValue = 1.0f / (weight0 / vertex0.position.z + weight1 / vertex1.position.z + weight2 / vertex2.position.z); // non linear

					// Frustum culling : false = inside frustrum
					if (zBufferValue < 0.0f || zBufferValue > 1.0f) continue;
					
					// Depth test : false = pixel infront
					int bufferIdx = px + py * m_Width;
					if (zBufferValue > m_pDepthBufferPixels[bufferIdx]) continue;

					// for color / UV/ normals...  use position.w instead of position.z
					float viewSpaceDepth = 1.0f / (weight0 / vertex0.position.w + weight1 / vertex1.position.w + weight2 / vertex2.position.w); //  linear


					ColorRGB color{ vertex0.color * weight0 + vertex1.color * weight1 + vertex2.color * weight2 };

					if (m_DebugDepthBuffer)
					{
						color.r = color.g = color.b = zBufferValue;
					}

					color.MaxToOne();


					//Vector2 uv = (vertex0.uv / vertex0.position.w * weight0 + vertex1.uv / vertex1.position.w * weight1 + vertex2.uv / vertex2.position.w * weight2) * viewSpaceDepth;
					//if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) continue;
					//color *= m_pTestTexture->Sample(uv);

					WritePixel(px, py, color, viewSpaceDepth);
				}
			}
	}

	bool Renderer::IsPixelInTriangle(const Vector2& point, const Vertex_Out& vertex0, const Vertex_Out& vertex1, const Vertex_Out& vertex2, float& weight0, float& weight1, float& weight2, const Triangle& triangle) const
	{
		Vector2 pointToVertex0 = point - vertex0.position.GetXY();
		Vector2 pointToVertex1 = point - vertex1.position.GetXY();
		Vector2 pointToVertex2 = point - vertex2.position.GetXY();

		// Calculate areas : must have the same sign 
		float area0 = Vector2::Cross(triangle.edge1, pointToVertex1);
		if (std::signbit(area0) != std::signbit(triangle.totalArea))
			return false;

		float area1 = Vector2::Cross(triangle.edge2, pointToVertex2);
		if (std::signbit(area1) != std::signbit(triangle.totalArea))
			return false;

		float area2 = Vector2::Cross(triangle.edge0, pointToVertex0);
		if (std::signbit(area2) != std::signbit(triangle.totalArea))
			return false;

		weight0 = area0 * triangle.invTotalArea;
		weight1 = area1 * triangle.invTotalArea;
		weight2 = area2 * triangle.invTotalArea;

		return true;
	}



	bool Renderer::PerformDepthTest(int px, int py, float zBufferValue) const
	{
		int idx = px + py * m_Width;
		assert(idx < m_Width * m_Height && "index out of bounds");
		return zBufferValue < m_pDepthBufferPixels[idx];
	}

	void Renderer::WritePixel(int px, int py, const ColorRGB& color, float zBufferValue)
	{
		int idx = px + py * m_Width;
		assert(idx < m_Width * m_Height && "index out of bounds");

		m_pBackBufferPixels[idx] = SDL_MapRGB(
			m_pBackBuffer->format,
			static_cast<uint8_t>(color.r * 255),
			static_cast<uint8_t>(color.g * 255),
			static_cast<uint8_t>(color.b * 255)
		);

		m_pDepthBufferPixels[idx] = zBufferValue;
	}

	

	void Renderer::TransformToViewSpace(Vertex_Out& vertex) const
	{
		vertex.position = m_Camera.viewMatrix.TransformPoint(vertex.position);
	}

	void Renderer::TransformToScreenSpace(Vertex_Out& vertex) const
	{
		vertex.position.x = (vertex.position.x + 1) * 0.5f * m_Width;
		vertex.position.y = (1 - vertex.position.y) * 0.5f * m_Height;
	}

	void Renderer::PerspectiveDivide(Vertex_Out& vertex) const
	{
		vertex.position.x /= vertex.position.w;
		vertex.position.y /= vertex.position.w;
		vertex.position.z /= vertex.position.w;
	}

	void Renderer::SetCameraSettings(Vertex_Out& vertex) const
	{
		vertex.position.x /= m_Camera.aspectRatio * m_Camera.fov;
		vertex.position.y /= m_Camera.fov;
	}

	Triangle Renderer::CalculateTriangleData(const Vertex_Out& vertex0, const Vertex_Out& vertex1, const Vertex_Out& vertex2)
	{
		Triangle data;
		data.edge0 = (vertex1.position - vertex0.position).GetXY();
		data.edge1 = (vertex2.position - vertex1.position).GetXY();
		data.edge2 = (vertex0.position - vertex2.position).GetXY();

		data.totalArea = Vector2::Cross(data.edge0, -data.edge2);
		data.invTotalArea = 1 / data.totalArea;

		return data;
	}


	void Renderer::ToggleDebugDepthBuffer()
	{
		m_DebugDepthBuffer = !m_DebugDepthBuffer;
	}

}
