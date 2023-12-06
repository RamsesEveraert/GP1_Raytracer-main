//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Texture.h"
#include "Utils.h"
#include "Maths.h"
#include "BRDFs.h"

namespace dae
{
	Renderer::Renderer(SDL_Window* pWindow) 
		: m_pWindow(pWindow)
		, m_CurrentLightingMode{ LightingMode::Combined }
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
		m_AspectRatio = static_cast<float>(m_Width) / m_Height;

		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;
		m_pDepthBufferPixels = new float[m_Width * m_Height] {};

		//Initialize Camera
		m_Camera.aspectRatio = m_AspectRatio;
		m_Camera.Initialize(45.f, { 0.0f,5.f,-64.f }, 0.1f, 100.f);
	

		// Initialize test mesh & texture(s)
		Utils::ParseOBJ("Resources/vehicle.obj", m_Mesh.vertices, m_Mesh.indices);
		m_Mesh.primitiveTopology = PrimitiveTopology::TriangleList;			
		m_Mesh.translateMatrix = Matrix::CreateTranslation(0.f, 0.f, 0.f);

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
			const float rotationSpeed = 0.8f;
			m_Mesh.rotateMatrix *= Matrix::CreateRotationY(rotationSpeed * pTimer->GetElapsed());
		}
		m_Mesh.UpdateWorldMatrix();
	}

	void Renderer::Render()
	{
		//@START
		std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
		SDL_FillRect(m_pBackBuffer, nullptr, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

		//Lock BackBuffer
		SDL_LockSurface(m_pBackBuffer);

		// Assignment
		RenderMesh();

		//@END
		//Update SDL Surface
		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);
	}

	void Renderer::RenderMesh()
	{
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
	}


	void Renderer::VertexTransformationFunction(Mesh& mesh) const
	{

		// match size of vertices_out with the max vertices of the mesh
		mesh.vertices_out.resize(mesh.vertices.size());

		// Calculate WorldViewProjection matrix
		Matrix worldViewProjectionMatrix = mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;

		for (size_t i = 0; i < mesh.vertices.size(); ++i)
		{
			// Convert Vertex to Vertex_Out
			mesh.vertices_out[i].position = {mesh.vertices[i].position, 1.0f};
			mesh.vertices_out[i].color = mesh.vertices[i].color;
			mesh.vertices_out[i].uv = mesh.vertices[i].uv;
			mesh.vertices_out[i].normal = mesh.worldMatrix.TransformVector(mesh.vertices[i].normal); // normals in worldview
			mesh.vertices_out[i].tangent = mesh.worldMatrix.TransformVector(mesh.vertices[i].tangent); // tangent in worldview

			mesh.vertices_out[i].position = worldViewProjectionMatrix.TransformPoint(mesh.vertices_out[i].position); // projection view for vertices

			Vector3 worldPosition = mesh.worldMatrix.TransformPoint(mesh.vertices_out[i].position);
			mesh.vertices_out[i].viewDirection = (worldPosition - m_Camera.origin).Normalized();

			PerspectiveDivide(mesh.vertices_out[i]);
			TransformToScreenSpace(mesh.vertices_out[i]);
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
		for (size_t currentIdx = 2; currentIdx < mesh.indices.size(); ++currentIdx) //start odd triangle
		{
			// Check first vertex idx for every odd triangle
			size_t firstVertexIndex = (currentIdx % 2) == 0 ? currentIdx - 2 : currentIdx - 1;
			// Check second vertex idx for every odd triangle
			size_t secondVertexIndex = (currentIdx % 2) == 0 ? currentIdx - 1 : currentIdx - 2;
			// Check third vertex idx for every odd triangle
			size_t thirdVertexIndex = currentIdx;

			const Vertex_Out& vertex0 = mesh.vertices_out[mesh.indices[firstVertexIndex]];
			const Vertex_Out& vertex1 = mesh.vertices_out[mesh.indices[secondVertexIndex]];
			const Vertex_Out& vertex2 = mesh.vertices_out[mesh.indices[thirdVertexIndex]];

			RasterizeTriangle(vertex0, vertex1, vertex2);
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

	void Renderer::RasterizeTriangle(const Vertex_Out& vertex0, const Vertex_Out& vertex1, const Vertex_Out& vertex2)
	{
		if (vertex0.position.w < 0.0f || vertex1.position.w < 0.0f || vertex2.position.w < 0.0f) return;

		// Find triangle bounding box
		int minBoxX = static_cast<int>(std::floor(std::min({ vertex0.position.x, vertex1.position.x, vertex2.position.x })));
		int maxBoxX = static_cast<int>(std::ceil(std::max({ vertex0.position.x, vertex1.position.x, vertex2.position.x })));
		int minBoxY = static_cast<int>(std::floor(std::min({ vertex0.position.y, vertex1.position.y, vertex2.position.y })));
		int maxBoxY = static_cast<int>(std::ceil(std::max({ vertex0.position.y, vertex1.position.y, vertex2.position.y })));

		// Clamping to the screen size
		minBoxX = std::clamp(minBoxX, 0, m_Width);
		maxBoxX = std::clamp(maxBoxX, 0, m_Width);
		minBoxY = std::clamp(minBoxY, 0, m_Height);
		maxBoxY = std::clamp(maxBoxY, 0, m_Height);

		// Calculate triangle edges
		const Vector2 edge0 = (vertex1.position - vertex0.position).GetXY();
		const Vector2 edge1 = (vertex2.position - vertex1.position).GetXY();
		const Vector2 edge2 = (vertex0.position - vertex2.position).GetXY();

		const float invTotalWeight = 1.0f / Vector2::Cross(edge0, -edge2);

		// Loop variables
		int pixelIndex = -1;
		Vector2 pixel, point0, point1, point2;
		float weight0, weight1, weight2;
		float depthZ, depthW;

		Vertex_Out pixelVertex;

		for (int px = minBoxX; px < maxBoxX; ++px)
		{
			for (int py = minBoxY; py < maxBoxY; ++py)
			{
				pixel.x = px + 0.5f;
				pixel.y = py + 0.5f;

				// Calculate vertex to pixel vectors
				point0 = pixel - vertex0.position.GetXY();
				point1 = pixel - vertex1.position.GetXY();
				point2 = pixel - vertex2.position.GetXY();

				// Barycentric cooridnates
				weight0 = Vector2::Cross(edge1, point1) * invTotalWeight;
				weight1 = Vector2::Cross(edge2, point2) * invTotalWeight;
				weight2 = Vector2::Cross(edge0, point0) * invTotalWeight;

				// Check sign equality
				if (weight0 < 0.0f || weight1 < 0.0f || weight2 < 0.0f) continue;

				// Interpolate depth Z value using weights
				depthZ = 1.0f / (weight0 / vertex0.position.z + weight1 / vertex1.position.z + weight2 / vertex2.position.z);

				// Frustum culling
				if (depthZ < 0.0f || depthZ > 1.0f) continue;

				// Calculate pixel index
				pixelIndex = px + py * m_Width;
				assert(pixelIndex < m_Width * m_Height && "buffer index out of bounds");

				// Depth test
				if (depthZ > m_pDepthBufferPixels[pixelIndex]) continue;

				m_pDepthBufferPixels[pixelIndex] = depthZ;

				// Interpolate depth W value using weights
				depthW = 1.0f / (weight0 / vertex0.position.w + weight1 / vertex1.position.w + weight2 / vertex2.position.w);

				pixelVertex.position = { static_cast<float>(px), static_cast<float>(py), depthZ, depthW };
				pixelVertex.color = vertex0.color * weight0 + vertex1.color * weight1 + vertex2.color * weight2;
				pixelVertex.normal = ((vertex0.normal * weight0 + vertex1.normal * weight1 + vertex2.normal * weight2) / 3).Normalized();
				pixelVertex.tangent = (vertex0.tangent * weight0 + vertex1.tangent * weight1 + vertex2.tangent * weight2).Normalized();
				pixelVertex.viewDirection = (vertex0.viewDirection * weight0 + vertex1.viewDirection * weight1 + vertex2.viewDirection * weight2).Normalized();
				pixelVertex.uv = (vertex0.uv / vertex0.position.w * weight0 + vertex1.uv / vertex1.position.w * weight1 + vertex2.uv / vertex2.position.w * weight2) * depthW;

				ShadePixel(pixelIndex, pixelVertex, depthZ);
			}
		}
	}

	void Renderer::TransformToScreenSpace(Vertex_Out& vertex) const
	{
		vertex.position.x = (vertex.position.x + 1) * 0.5f * m_Width;
		vertex.position.y = (1 - vertex.position.y) * 0.5f * m_Height;
	}

	float Renderer::DepthRemap(const float depthz, const float min, const float max)
	{
		float remappedDepthZ = (depthz - min) / (max - min);
		if (remappedDepthZ < 0.f)
		{
			remappedDepthZ = 0.f;
		}
		return remappedDepthZ;
	}

	void Renderer::PerspectiveDivide(Vertex_Out& vertex) const
	{
		vertex.position.x /= vertex.position.w;
		vertex.position.y /= vertex.position.w;
		vertex.position.z /= vertex.position.w;
	}

	void Renderer::ShadePixel(int pixelIndex, const Vertex_Out& v, float depthz)
	{
		ColorRGB color = v.color;
		Vector3 normal = v.normal;

		// Normal mapping
		if (m_NormalMapping && m_pNormalTexture != nullptr)
		{
			Vector3 binormal = Vector3::Cross(v.normal, v.tangent);
			Matrix tangentSpaceMatrix = Matrix{ v.tangent, binormal, v.normal, Vector3::Zero };
			normal = tangentSpaceMatrix.TransformVector(m_pNormalTexture->SampleNormal(v.uv));
		}

		// Debug depth buffer
		if (m_DebugDepthBuffer)
		{
			float remap = DepthRemap(depthz, 0.9975f, 1.f);
			color = ColorRGB{ remap, remap, remap };
		}
		else
		{
			// Lighting calculations
			ColorRGB ambient = ColorRGB{ 0.03f, 0.03f, 0.03f };
			float cosLaw = std::max(Vector3::Dot(-m_GlobalLightDirection, normal), 0.0f);
			ColorRGB observedArea{ cosLaw, cosLaw, cosLaw };

			ColorRGB lambert{};
			ColorRGB phong{};

			switch (m_CurrentLightingMode)
			{
			case dae::Renderer::LightingMode::ObservedArea:
				color = observedArea;
				break;
			case dae::Renderer::LightingMode::Diffuse:
				if (m_pAlbedoTexture != nullptr)
				{
					const float kd = 7.f;
					lambert = BRDF::Lambert(kd, m_pAlbedoTexture->Sample(v.uv));
					color = lambert * observedArea;
				}
				break;
			case dae::Renderer::LightingMode::Specular:
				if (m_pGlossTexture != nullptr && m_pSpecularTexture != nullptr)
				{
					phong = BRDF::Phong(m_pSpecularTexture->SampleGray(v.uv), m_pGlossTexture->SampleGray(v.uv) * m_Shininess, -m_GlobalLightDirection, v.viewDirection, normal);
					color = phong * observedArea;
				}
				break;
			case dae::Renderer::LightingMode::Combined:
				if (m_pAlbedoTexture != nullptr)
				{
					const float kd = 7.f;
					lambert = BRDF::Lambert(kd, m_pAlbedoTexture->Sample(v.uv));
				}
				if (m_pGlossTexture != nullptr && m_pSpecularTexture != nullptr)
				{
					phong = BRDF::Phong(m_pSpecularTexture->SampleGray(v.uv), m_pGlossTexture->SampleGray(v.uv) * m_Shininess, -m_GlobalLightDirection, v.viewDirection, normal);
				}
				color = (lambert + phong) * observedArea;
				break;
			default:
				break;
			}

			color += ambient;
			color.MaxToOne();
		}

		m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(
			m_pBackBuffer->format,
			static_cast<uint8_t>(color.r * 255),
			static_cast<uint8_t>(color.g * 255),
			static_cast<uint8_t>(color.b * 255)
		);
	}


	void Renderer::CycleLightning()
	{
		int cyclePhase{ static_cast<int>(m_CurrentLightingMode) };
		cyclePhase = (cyclePhase + 1) % static_cast<int>(LightingMode::Max); // next + check in interval

		m_CurrentLightingMode = static_cast<LightingMode>(cyclePhase);
	}

}
