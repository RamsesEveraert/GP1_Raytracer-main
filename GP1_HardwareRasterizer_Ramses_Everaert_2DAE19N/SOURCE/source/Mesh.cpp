#include "pch.h"
#include "Mesh.h"

#include "Camera.h"
#include "Matrix.h"

#include <cassert>

namespace dae
{
	Mesh::Mesh(ID3D11Device* pDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
		: m_pDevice{ pDevice }
		, m_Vertices{ vertices }
		, m_Indices{ indices }
		, m_NumIndices{ static_cast<uint32_t>(indices.size()) }
		, m_pEffect{ std::make_unique<Effect>(pDevice, L"Resources/PosCol3D.fx") }
		, m_WorldMatrix{ Matrix::Identity() }
	{
		// create vertex buffer

		D3D11_BUFFER_DESC bufferDesc{};
		bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		bufferDesc.ByteWidth = sizeof(Vertex) * static_cast<uint32_t>(m_Vertices.size());
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData{};
		initData.pSysMem = m_Vertices.data();

		HRESULT result = m_pDevice->CreateBuffer(&bufferDesc, &initData, &m_pVertexBuffer);
		if (FAILED(result))
		{
			std::wcout << L"Failed to create vertex buffer\n";
			assert(false);
		}

		// Create index buffer

		bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		bufferDesc.ByteWidth = sizeof(uint32_t) * m_NumIndices;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		initData.pSysMem = m_Indices.data();

		result = m_pDevice->CreateBuffer(&bufferDesc, &initData, &m_pIndexBuffer);
		if (FAILED(result))
		{
			std::wcout << L"Failed to create index buffer\n";
			assert(false);
		}
	}

	Mesh::~Mesh()
	{
		if (m_pIndexBuffer) m_pIndexBuffer->Release();
		if (m_pVertexBuffer) m_pVertexBuffer->Release();
	}

	void Mesh::Render(Camera* camera, ID3D11DeviceContext* pDeviceContext) const
	{
		// 1. Set primitive Topology
		pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 2. Set input layout
		pDeviceContext->IASetInputLayout(m_pEffect->GetInputLayout());

		// 3. Set vertex buffer
		constexpr UINT stride = sizeof(Vertex);
		constexpr UINT offset = 0;
		pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

		// 4. Set Index buffer
		pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// 5. Set world and worldview Matrix
		Matrix worldMatrix = m_WorldMatrix; // must pass L-value
		Matrix worldViewMatrix = m_WorldMatrix * camera->viewMatrix * camera->projectionMatrix; // must pass L-value
		m_pEffect->SetWorldMatrix(worldMatrix);
		m_pEffect->SetWorldViewProjMatrix(worldViewMatrix);

		// 6. Set CameraPosition
		m_pEffect->SetCameraPosition(camera->origin);

		// 7. Set Shader Texture
		if (m_pDiffuseMap) m_pEffect->SetDiffuseMap(m_pDiffuseMap.get());
		if (m_pNormalMap) m_pEffect->SetNormalMap(m_pNormalMap.get());
		if (m_pSpecularMap) m_pEffect->SetSpecularMap(m_pSpecularMap.get());
		if (m_pGlossMap) m_pEffect->SetGlossinessMap(m_pGlossMap.get());

		// 7. Set DiffuseMap
		//m_pEffect->SetDiffuseMap(m_pDiffuseMap.get());

		// 8. Draw 
		D3DX11_TECHNIQUE_DESC techniqueDesc{};
		m_pEffect->GetTechnique()->GetDesc(&techniqueDesc);
		for (UINT i = 0; i < techniqueDesc.Passes; ++i)
		{
			m_pEffect->GetTechnique()->GetPassByIndex(i)->Apply(0, pDeviceContext);
			pDeviceContext->DrawIndexed(m_NumIndices, 0, 0);
		}
	}
	void Mesh::SetWorldMatrix(const Matrix& newMatrix)
	{
		m_WorldMatrix = newMatrix;
	}
	const Matrix& Mesh::GetWorldMatrix() const
	{
		return m_WorldMatrix;
	}
	void Mesh::SetDiffuseMap(const std::string& filepath)
	{
		m_pDiffuseMap = std::make_unique<Texture>(m_pDevice, filepath);
	}

	void Mesh::SetNormalMap(const std::string& filepath)
	{
		m_pNormalMap = std::make_unique<Texture>(m_pDevice, filepath);
	}

	void Mesh::SetSpecularMap(const std::string& filepath)
	{
		m_pSpecularMap = std::make_unique<Texture>(m_pDevice, filepath);
	}

	void Mesh::SetGlossinessMap(const std::string& filepath)
	{
		m_pGlossMap = std::make_unique<Texture>(m_pDevice, filepath);
	}
}