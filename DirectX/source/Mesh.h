#pragma once
#include "Effect.h"
#include "Texture.h"

#include <memory>
namespace dae
{
	struct Vertex_PosCol
	{
		Vector3 position;
		ColorRGB color;
	};

	struct Camera;
	struct Matrix;
	class Mesh final
	{
	public:
		Mesh(ID3D11Device* pDevice, const std::vector<Vertex_PosCol>& vertices, const std::vector<uint32_t>& indices);
		~Mesh();

		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;
		Mesh(Mesh&&) = delete;
		Mesh& operator=(Mesh&&) = delete;

		void Render(Camera* camera, ID3D11DeviceContext* pDeviceContext) const;
		void SetWorldMatrix(const Matrix& newMatrix);

		void SetDiffuseMap(const std::string& filepath);

	private:
		std::vector<Vertex_PosCol> m_Vertices;
		std::vector<uint32_t> m_Indices;
		uint32_t m_NumIndices;

		std::unique_ptr<Effect> m_pEffect;
		std::unique_ptr<Texture> m_pDiffuseMap;

		Matrix m_WorldMatrix;

		ID3D11Buffer* m_pVertexBuffer;
		ID3D11Buffer* m_pIndexBuffer;

		ID3D11Device* m_pDevice;
	};
}