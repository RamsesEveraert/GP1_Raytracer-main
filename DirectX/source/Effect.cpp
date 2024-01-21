#include "pch.h"
#include "Effect.h"

#include "Texture.h"

#include <cassert>


using namespace dae;

Effect::Technique Effect::m_CurrentTechnique{ Technique::POINT };

Effect::Effect(ID3D11Device* pDevice, const std::wstring& assetFile)
	: m_pEffect{ LoadEffect(pDevice, assetFile) }
{
	m_pPointTechnique = GetTechnique("PointTechnique");
	m_pLinearTechnique = GetTechnique("LinearTechnique");
	m_pAnisotropicTechnique = GetTechnique("AnisotropicTechnique");

	// create vertex layout
	static constexpr uint32_t numElements{ 3 };
	D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

	// A) 3 * 32 bits = 96 bits = 12 bytes (for 3 colors)
	vertexDesc[0].SemanticName = "POSITION";
	vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[0].AlignedByteOffset = 0; // start at 0
	vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	// C) 2 * 32 bits = 64 bits = 8 bytes  (for 2 coords)
	vertexDesc[1].SemanticName = "TEXCOORD";
	vertexDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	vertexDesc[1].AlignedByteOffset = 12; // Start at 12 see A)
	vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	// B) 3 * 32 bits = 96 bits = 12 bytes (for 3 colors)
	vertexDesc[2].SemanticName = "COLOR";
	vertexDesc[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[2].AlignedByteOffset = 20; // start at 20 see B)
	vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

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

	// Create ID3DX11EffectMatrixVariable 
	m_pMatWorldViewProjVariable = GetVariable("gWorldViewProj")->AsMatrix();
	if (!m_pMatWorldViewProjVariable->IsValid())
	{
		std::wcout << L"m_pMatWorldViewProjVariable not valid!\n";
	}

	// Create ID3DX11EffectShaderResource variable
	m_pDiffuseMapVariable = GetVariable("gDiffuseMap")->AsShaderResource();
	if (!m_pMatWorldViewProjVariable->IsValid())
	{
		std::wcout << L"m_pMatWorldViewProjVariable not valid!\n";
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
void Effect::SwitchTechnique()
{
	switch (m_CurrentTechnique)
	{
	case Technique::POINT:
		m_CurrentTechnique = Technique::LINEAR;
		break;

	case Technique::LINEAR:
		m_CurrentTechnique = Technique::ANISOTROPIC;
		break;

	case Technique::ANISOTROPIC:
		m_CurrentTechnique = Technique::POINT;
		break;
	}
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
