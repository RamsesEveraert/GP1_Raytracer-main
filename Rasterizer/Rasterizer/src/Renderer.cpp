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
		m_Camera.aspectRatio = static_cast<float>(m_Width) / m_Height;

		// Initialize test mesh & texture(s)
		Utils::ParseOBJ("Resources/vehicle.obj", m_Mesh.vertices, m_Mesh.indices);
		m_Mesh.primitiveTopology = PrimitiveTopology::TriangleList;

		m_pAlbedoTexture = Texture::LoadFromFile("Resources/vehicle_diffuse.png");
		m_pNormalTexture = Texture::LoadFromFile("Resources/vehicle_normal.png");
		m_pGlossTexture = Texture::LoadFromFile("Resources/vehicle_gloss.png");
		m_pSpecularTexture = Texture::LoadFromFile("Resources/vehicle_specular.png");
	}

	Renderer::~Renderer()
	{
		delete[] m_pDepthBufferPixels;
		delete m_pAlbedoTexture;
		delete m_pNormalTexture;
		delete m_pGlossTexture;
		delete m_pSpecularTexture;

	}

	void Renderer::Update(Timer* pTimer)
	{
		m_Camera.Update(pTimer);

		if (m_IsMeshRotating)
		{
			const float rotationSpeed = 0.5f;
			m_Mesh.worldMatrix *= Matrix::CreateRotationY(rotationSpeed * pTimer->GetElapsed());
		}
	}

	void Renderer::Render()
	{
		//@START
		std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
		SDL_FillRect(m_pBackBuffer, nullptr, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

		//Lock BackBuffer
		SDL_LockSurface(m_pBackBuffer);

		// Render Test mesh
		VertexTransformationFunction(m_Mesh);

		switch (m_Mesh.primitiveTopology)
		{
		case PrimitiveTopology::TriangleList:
			RasterizeTriangleList(m_Mesh);
			break;

		case PrimitiveTopology::TriangleStrip:
			RasterizeTriangleStrip(m_Mesh);
			break;
		}

		//@END
		//Update SDL Surface
		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);
	}

	void Renderer::VertexTransformationFunction(Mesh& mesh) const
	{
		const auto& verticesIn = mesh.vertices;
		auto& verticesOut = mesh.vertices_out;

		Matrix worldViewProjectionMatrix = mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;

		verticesOut.resize(verticesIn.size());

		for (size_t i = 0; i < verticesIn.size(); ++i)
		{
			Vertex_Out& vertex = verticesOut[i];

			// Convert Vertex to Vertex_Out
			vertex.position = { verticesIn[i].position, 0.0f };
			vertex.color = verticesIn[i].color;
			vertex.uv = verticesIn[i].uv;
			vertex.normal = mesh.worldMatrix.TransformVector(verticesIn[i].normal);
			vertex.tangent = mesh.worldMatrix.TransformVector(verticesIn[i].tangent);

			vertex.position = worldViewProjectionMatrix.TransformPoint(vertex.position);

			Vector3 worldPosition = mesh.worldMatrix.TransformPoint(vertex.position);
			vertex.viewDirection = (worldPosition - m_Camera.origin).Normalized();

			// Perspective divide
			vertex.position.x /= vertex.position.w;
			vertex.position.y /= vertex.position.w;
			vertex.position.z /= vertex.position.w;

			// Transform to screen space
			vertex.position.x = (vertex.position.x + 1) * 0.5f * m_Width;
			vertex.position.y = (1 - vertex.position.y) * 0.5f * m_Height;
		}
	}

	bool Renderer::SaveBufferToImage() const
	{
		return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
	}

	void Renderer::ToggleDebugDepthBuffer()
	{
		m_DebugDepthBuffer = !m_DebugDepthBuffer;
	}

	void Renderer::ToggleDebugRotation()
	{
		m_IsMeshRotating = !m_IsMeshRotating;
	}

	void Renderer::ToggleNormalMapping()
	{
		m_NormalMapping = !m_NormalMapping;
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

	void Renderer::RasterizeTriangle(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2)
	{
		if (v0.position.w < 0.0f || v1.position.w < 0.0f || v2.position.w < 0.0f) return;

		// Find triangle bounding box
		auto boxLeft = static_cast<int>(std::min({ v0.position.x, v1.position.x, v2.position.x })) - 1;
		auto boxTop = static_cast<int>(std::min({ v0.position.y, v1.position.y, v2.position.y })) - 1;
		auto boxRight = static_cast<int>(std::max({ v0.position.x, v1.position.x, v2.position.x })) + 1;
		auto boxBottom = static_cast<int>(std::max({ v0.position.y, v1.position.y, v2.position.y })) + 1;

		boxLeft = std::max(0, boxLeft);
		boxTop = std::max(0, boxTop);
		boxRight = std::min(m_Width, boxRight);
		boxBottom = std::min(m_Height, boxBottom);

		// Calculate triangle edges
		const Vector2 e0 = (v1.position - v0.position).GetXY();
		const Vector2 e1 = (v2.position - v1.position).GetXY();
		const Vector2 e2 = (v0.position - v2.position).GetXY();

		const float invTotalWeight = 1.0f / Vector2::Cross(e0, -e2);

		// Loop variables
		int pixelIndex = -1;
		Vector2 pixel, p0, p1, p2;
		float w0, w1, w2;
		float depthZ, depthW;

		Vertex_Out pixelVertex;

		for (int px = boxLeft; px < boxRight; ++px)
		{
			for (int py = boxTop; py < boxBottom; ++py)
			{
				pixel.x = px + 0.5f;
				pixel.y = py + 0.5f;

				// Calculate vertex to pixel vectors
				p0 = pixel - v0.position.GetXY();
				p1 = pixel - v1.position.GetXY();
				p2 = pixel - v2.position.GetXY();

				// Barycentric cooridnates (weights)
				w0 = Vector2::Cross(e1, p1) * invTotalWeight;
				w1 = Vector2::Cross(e2, p2) * invTotalWeight;
				w2 = Vector2::Cross(e0, p0) * invTotalWeight;

				// Check sign equality
				if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) continue;

				// Interpolate depth Z value using weights
				depthZ = 1.0f / (w0 / v0.position.z + w1 / v1.position.z + w2 / v2.position.z);

				// Frustum culling
				if (depthZ < 0.0f || depthZ > 1.0f) continue;

				// Calculate pixel index
				pixelIndex = px + py * m_Width;
				assert(pixelIndex < m_Width * m_Height && "buffer index out of bounds");

				// Depth test
				if (depthZ > m_pDepthBufferPixels[pixelIndex]) continue;

				m_pDepthBufferPixels[pixelIndex] = depthZ;

				// Interpolate depth W value using weights
				depthW = 1.0f / (w0 / v0.position.w + w1 / v1.position.w + w2 / v2.position.w);

				pixelVertex.position = { static_cast<float>(px), static_cast<float>(py), depthZ, depthW };
				pixelVertex.color = v0.color * w0 + v1.color * w1 + v2.color * w2;
				pixelVertex.normal = (v0.normal * w0 + v1.normal * w1 + v2.normal * w2).Normalized();
				pixelVertex.tangent = (v0.tangent * w0 + v1.tangent * w1 + v2.tangent * w2).Normalized();
				pixelVertex.viewDirection = (v0.viewDirection * w0 + v1.viewDirection * w1 + v2.viewDirection * w2).Normalized();
				pixelVertex.uv = (v0.uv / v0.position.w * w0 + v1.uv / v1.position.w * w1 + v2.uv / v2.position.w * w2) * depthW;

				ShadePixel(pixelIndex, pixelVertex);
			}
		}
	}

	void Renderer::ShadePixel(int pixelIndex, const Vertex_Out& v) const
	{
		ColorRGB color = v.color;
		Vector3 normal = v.normal;

		if (m_NormalMapping && m_pNormalTexture != nullptr)
		{
			Vector3 binoral = Vector3::Cross(v.normal, v.tangent);
			Matrix tangentSpaceMatrix = Matrix{ v.tangent, binoral, v.normal, Vector3::Zero };
			normal = tangentSpaceMatrix.TransformVector(m_pNormalTexture->SampleNormal(v.uv));
		}

		float observedArea = std::max(Vector3::Dot(-m_GlobalLightDirection, normal), 0.0f);

		if (m_DebugDepthBuffer)
		{
			color.r = color.g = color.b = v.position.z;
		}
		else
		{
			if (m_pAlbedoTexture != nullptr)
			{
				color = m_pAlbedoTexture->Sample(v.uv);
			}

			if (m_pGlossTexture != nullptr && m_pSpecularTexture != nullptr)
			{
				float cosa = Vector3::Dot(Vector3::Reflect(-m_GlobalLightDirection, normal), v.viewDirection);
				if (cosa > 0.0f)
				{
					float phong = m_pSpecularTexture->SampleGray(v.uv) * std::pow(cosa, m_pGlossTexture->SampleGray(v.uv) * m_Shininess);
					color += { phong, phong, phong };
				}
			}

			color *= observedArea;
			color.MaxToOne();
		}

		m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(
			m_pBackBuffer->format,
			static_cast<uint8_t>(color.r * 255),
			static_cast<uint8_t>(color.g * 255),
			static_cast<uint8_t>(color.b * 255)
		);
	}
}
