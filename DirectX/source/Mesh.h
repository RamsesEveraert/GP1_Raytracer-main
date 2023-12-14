#pragma once

namespace dae
{
	struct Vertex_PosCol
	{
		Vector3 position;
		ColorRGB color;
	};

	class Effect;

	class Mesh final
	{
	public:
		Mesh(ID3D11Device* pDevice, const std::vector<Vertex_PosCol>& vertices, const std::vector<uint32_t>& indices);
		~Mesh();

		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;
		Mesh(Mesh&&) = delete;
		Mesh& operator=(Mesh&&) = delete;

		void Render(ID3D11DeviceContext* pDeviceContext) const;

	private:
		std::vector<Vertex_PosCol> m_Vertices;
		std::vector<uint32_t> m_Indices;
		uint32_t m_NumIndices;

		Effect* m_pEffect;

		ID3D11Buffer* m_pVertexBuffer;
		ID3D11Buffer* m_pIndexBuffer;
	};
}