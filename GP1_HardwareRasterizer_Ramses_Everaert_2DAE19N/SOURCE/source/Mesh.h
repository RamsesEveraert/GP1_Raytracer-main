#pragma once
#include "Effect.h"
#include "Texture.h"

#include <memory>
namespace dae
{
	struct Vertex
	{
		Vector3 position;
		Vector2 texCoord;
		Vector3 normal;
		Vector3 tangent;
	};

	struct Camera;
	struct Matrix;
	class Mesh final
	{
	public:
		Mesh(ID3D11Device* pDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
		~Mesh();

		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;
		Mesh(Mesh&&) = delete;
		Mesh& operator=(Mesh&&) = delete;

		void Render(Camera* camera, ID3D11DeviceContext* pDeviceContext) const;

		void SetWorldMatrix(const Matrix& newMatrix);
		const Matrix& GetWorldMatrix() const;

		void SetDiffuseMap(const std::string& filepath);
		void SetNormalMap(const std::string& filepath);
		void SetSpecularMap(const std::string& filepath);
		void SetGlossinessMap(const std::string& filepath);

	private:

		//  Mesh variables
		std::vector<Vertex> m_Vertices;
		std::vector<uint32_t> m_Indices;
		uint32_t m_NumIndices;

		ID3D11Buffer* m_pVertexBuffer;
		ID3D11Buffer* m_pIndexBuffer;
		ID3D11Device* m_pDevice;

		// Texture Variables
		std::unique_ptr<Effect> m_pEffect;

		std::unique_ptr<Texture> m_pDiffuseMap;
		std::unique_ptr<Texture> m_pNormalMap;
		std::unique_ptr<Texture> m_pSpecularMap;
		std::unique_ptr<Texture> m_pGlossMap;

		// World variables
		Matrix m_WorldMatrix;

	};
}