#include "pch.h"
#include "Effect.h"

#include "Texture.h"

#include <cassert>


using namespace dae;

Effect::Technique Effect::m_CurrentTechnique{ Technique::POINT };

Effect::Effect(ID3D11Device* pDevice, const std::wstring& assetFile)
	: m_pEffect{ LoadEffect(pDevice, assetFile) }
{
	//=====================  Get the techniques and store them in member variables  ================//

	m_pPointTechnique = GetTechnique("PointTechnique");
	m_pLinearTechnique = GetTechnique("LinearTechnique");
	m_pAnisotropicTechnique = GetTechnique("AnisotropicTechnique");

	//=====================  Get the Variables and store them in member variables  ================//

	// Get World matrix variables
	m_pMatWorldViewProjVariable = GetVariable("gWorldViewProj")->AsMatrix();
	m_pMatWorldVariable = GetVariable("gWorldMatrix")->AsMatrix();

	// Get Vector variables
	m_pCameraPositionVariable = GetVariable("gCameraPosition")->AsVector();

	// Get Shader Texture Resource variables
	m_pDiffuseMapVariable = GetVariable("gDiffuseMap")->AsShaderResource();
	m_pNormalMapVariable = GetVariable("gNormalMap")->AsShaderResource();
	m_pSpecularMapVariable = GetVariable("gSpecularMap")->AsShaderResource();
	m_pGlossinessMapVariable = GetVariable("gGlossMap")->AsShaderResource();

	//======================  Create Vertex Input Layout  ====================================//

	static constexpr uint32_t numElements{ 4 };
	D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

	// 3 * 32 bits = 96 bits = 12 bytes (3 components format of 32 bits)
	vertexDesc[0].SemanticName = "POSITION";
	vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[0].AlignedByteOffset = 0;
	vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	// 2 * 32 bits = 64 bits = 8 bytes  (2 components format of 32 bits)
	vertexDesc[1].SemanticName = "TEXCOORD";
	vertexDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	vertexDesc[1].AlignedByteOffset = 12; // position + 12 bytes
	vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	// 3 * 32 bits = 96 bits = 12 bytes (3 components format of 32 bits)
	vertexDesc[2].SemanticName = "NORMAL";
	vertexDesc[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[2].AlignedByteOffset = 20; // texcoord + 8 bytes
	vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	// 3 * 32 bits = 96 bits = 12 bytes (3 components format of 32 bits) 
	vertexDesc[3].SemanticName = "TANGENT";
	vertexDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[3].AlignedByteOffset = 32; // NORMAL + 12 bytes
	vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	//Create Input layout
	D3DX11_PASS_DESC passDesc{};
	m_pPointTechnique->GetPassByIndex(0)->GetDesc(&passDesc);

	HRESULT result = pDevice->CreateInputLayout(
		vertexDesc,
		numElements,
		passDesc.pIAInputSignature,
		passDesc.IAInputSignatureSize,
		&m_pInputLayout
	);

	if (FAILED(result))
	{
		std::wcout << L"Failed to create input layout\n";
		assert(false);
	}

}

Effect::~Effect()
{
	// reversed order!
	if (m_pInputLayout) m_pInputLayout->Release();
	if (m_pEffect) m_pEffect->Release();
}

ID3DX11Effect* Effect::GetEffect() const
{
	return m_pEffect;
}

ID3DX11EffectTechnique* Effect::GetTechnique() const
{
	if (m_CurrentTechnique == Technique::POINT) return m_pPointTechnique;
	else if (m_CurrentTechnique == Technique::LINEAR) return m_pLinearTechnique;
	else if (m_CurrentTechnique == Technique::ANISOTROPIC) return m_pAnisotropicTechnique;

	// fallback
	return m_pPointTechnique;
}

ID3DX11Effect* Effect::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
{
	HRESULT result;
	ID3D10Blob* pErrorBlob{ nullptr };
	ID3DX11Effect* pEffect{ nullptr };

	DWORD shaderFlags = 0;
#if defined(_DEBUG) || defined(DEBUG)
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	result = D3DX11CompileEffectFromFile(
		assetFile.c_str(),
		nullptr,
		nullptr,
		shaderFlags,
		0,
		pDevice,
		&pEffect,
		&pErrorBlob);

	if (FAILED(result))
	{
		if (pErrorBlob != nullptr)
		{
			const char* pErrors = static_cast<char*>(pErrorBlob->GetBufferPointer());

			std::wstringstream ss;
			for (unsigned int i = 0; i < pErrorBlob->GetBufferSize(); ++i)
			{
				ss << pErrors[i];
			}

			OutputDebugStringW(ss.str().c_str());

			pErrorBlob->Release();
			pErrorBlob = nullptr;

			std::wcout << ss.str() << std::endl;
		}
		else
		{
			std::wstringstream ss;
			ss << "EffectLoader: Failed to CreateEffectFromFile!\nPath: " << assetFile;
			std::wcout << ss.str() << std::endl;
		}

		return nullptr;
	}

	return pEffect;
}

ID3D11InputLayout* Effect::GetInputLayout() const
{
	return m_pInputLayout;
}

void Effect::SetCameraPosition(Vector3& position)
{
	m_pCameraPositionVariable->SetFloatVector(reinterpret_cast<float*>(&position.x));
}

void dae::Effect::SetWorldMatrix(Matrix& matrix)
{
	m_pMatWorldVariable->SetMatrix(reinterpret_cast<float*>(&matrix));
}

void Effect::SetWorldViewProjMatrix(Matrix& matrix)
{
	m_pMatWorldViewProjVariable->SetMatrix(reinterpret_cast<float*>(&matrix));
}

void Effect::SetDiffuseMap(const Texture* pTexture)
{
	if (m_pDiffuseMapVariable && pTexture)
	{
		m_pDiffuseMapVariable->SetResource(pTexture->GetShaderResourceView());
	}
}

void Effect::SetNormalMap(const Texture* pTexture)
{
	if (m_pNormalMapVariable && pTexture)
	{
		m_pNormalMapVariable->SetResource(pTexture->GetShaderResourceView());
	}
}

void Effect::SetSpecularMap(const Texture* pTexture)
{
	if (m_pSpecularMapVariable && pTexture)
	{
		m_pSpecularMapVariable->SetResource(pTexture->GetShaderResourceView());
	}
}

void Effect::SetGlossinessMap(const Texture* pTexture)
{
	if (m_pGlossinessMapVariable && pTexture)
	{
		m_pGlossinessMapVariable->SetResource(pTexture->GetShaderResourceView());
	}
}
void Effect::SwitchTechnique()
{
	std::string pTechniqueName{};
	switch (m_CurrentTechnique)
	{
	case Technique::POINT:
		m_CurrentTechnique = Technique::LINEAR;
		pTechniqueName = "Linear";
		break;

	case Technique::LINEAR:
		m_CurrentTechnique = Technique::ANISOTROPIC;
		pTechniqueName = "Anisotropic";
		break;

	case Technique::ANISOTROPIC:
		m_CurrentTechnique = Technique::POINT;
		pTechniqueName = "Point";
		break;
	}
	std::cout << "Sampling Method changed to: " << pTechniqueName << '\n';
}


ID3DX11EffectTechnique* Effect::GetTechnique(const std::string& name) const
{
	ID3DX11EffectTechnique* pTechnique = m_pEffect->GetTechniqueByName(name.data());
	if (!pTechnique->IsValid())
	{
		std::cout << "Couldn't find \"" << name << "\" technique!\n";
		assert(false);
	}
	return pTechnique;
}

ID3DX11EffectVariable* Effect::GetVariable(const std::string& name) const
{
	ID3DX11EffectVariable* pVariable = m_pEffect->GetVariableByName(name.data());
	if (!pVariable->IsValid())
	{
		std::cout << "Couldn't find \"" << name << "\n as shader variable!\n";
		assert(false);
	}
	return pVariable;
}
