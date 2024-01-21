#pragma once
#include "Matrix.h"

namespace dae
{
	class Texture;
	class Effect final
	{
	public:

		enum class Technique
		{
			POINT,
			LINEAR,
			ANISOTROPIC
		};

		Effect(ID3D11Device* pDevice, const std::wstring& assetFile);
		~Effect();

		Effect(const Effect&) = delete;
		Effect& operator=(const Effect&) = delete;
		Effect(Effect&&) = delete;
		Effect& operator=(Effect&&) = delete;

		ID3DX11Effect* GetEffect() const;
		ID3DX11EffectTechnique* GetTechnique() const;
		ID3D11InputLayout* GetInputLayout() const;

		void SetWorldViewProjMatrix(Matrix& matrix);
		void SetDiffuseMap(const Texture* pTexture);

		static void SwitchTechnique();

	private:
		ID3DX11Effect* m_pEffect;
		ID3D11InputLayout* m_pInputLayout;

		// Techniques
		ID3DX11EffectTechnique* m_pPointTechnique{ nullptr };
		ID3DX11EffectTechnique* m_pLinearTechnique{ nullptr };
		ID3DX11EffectTechnique* m_pAnisotropicTechnique{ nullptr };

		// FX variables
		ID3DX11EffectMatrixVariable* m_pMatWorldViewProjVariable{ nullptr };
		ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable{ nullptr };

		static Technique m_CurrentTechnique;

	private:
		static ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile);

		ID3DX11EffectTechnique* GetTechnique(const std::string& name) const;
		ID3DX11EffectVariable* GetVariable(const std::string& name) const;

	};
}