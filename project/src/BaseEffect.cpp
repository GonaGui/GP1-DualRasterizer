#include "BaseEffect.h"

#include <d3dcompiler.h>
#include <iostream>
#include <ostream>
#include <sstream>

#include "Material.h"

BaseEffect::BaseEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
{
	m_pEffect = LoadEffect(pDevice, assetFile);

	m_pMatWorldViewProjVariable = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();

	if (!m_pMatWorldViewProjVariable->IsValid())
		std::wcout << L"m_pMatWorldViewProjVariable not valid \n";


}

BaseEffect::~BaseEffect()
{

	if (m_pTechnique)
	{
		m_pTechnique->Release();
		m_pTechnique = nullptr;
	}

	if (m_pMatWorldViewProjVariable)
	{
		m_pMatWorldViewProjVariable->Release();
		m_pMatWorldViewProjVariable = nullptr;
	}

	if (m_pWorldMatrixVariable)
	{
		m_pWorldMatrixVariable->Release();
		m_pWorldMatrixVariable = nullptr;
	}

	if (m_pCameraPosition)
	{
		m_pCameraPosition->Release();
		m_pCameraPosition = nullptr;
	}

	if (m_pEffect)
	{
		m_pEffect->Release();
		m_pEffect = nullptr;
	}

	delete m_Material;
}

ID3DX11Effect* BaseEffect::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
{
	HRESULT result;
	ID3D10Blob* pErrorBlob{ nullptr };
	ID3DX11Effect* pEffect;

	DWORD shaderFlags = 0;

#if defined(DEBUG) || defined( _DEBUG )
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	result = D3DX11CompileEffectFromFile
	(
		assetFile.c_str(),
		nullptr,
		nullptr,
		shaderFlags,
		0,
		pDevice,
		&pEffect,
		&pErrorBlob
	);


	if (FAILED(result))
	{
		if (pErrorBlob != nullptr)
		{
			const char* pErrors = static_cast<char*>(pErrorBlob->GetBufferPointer());

			std::wstringstream ss;

			for (unsigned int i = 0; i < pErrorBlob->GetBufferSize(); i++)
				ss << pErrors[i];

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
			return nullptr;
		}
	}


	return pEffect;
}


ID3DX11Effect* BaseEffect::GetEffect() const
{
	return m_pEffect;
}

ID3DX11EffectTechnique* BaseEffect::GetTechnique() const
{
	return m_pTechnique;
}

const char* BaseEffect::GetCurrentTechnique() const
{
	return m_Techniques[m_CurrentTechnique];
}

void BaseEffect::UpdateData(const float* worldProjViewMatrix,const float* worldMatrix,const float* cameraPos) const
{
	m_pMatWorldViewProjVariable->SetMatrix(worldProjViewMatrix);

}


void BaseEffect::ToggleTechnique()
{
	m_CurrentTechnique += 1;

	if (m_CurrentTechnique > m_Techniques.size() - 1) m_CurrentTechnique = 0;

	m_pTechnique = m_pEffect->GetTechniqueByName(m_Techniques[m_CurrentTechnique]);

	if (!m_pTechnique->IsValid())
		std::wcout << L"Technique not valid\n";
}

void BaseEffect::SetMaterial(std::initializer_list<MatCompFormat>& materialComponent, ID3D11Device* directXDevice)
{
	m_Material = new Material(materialComponent, directXDevice);

	for (const auto matComp : m_Material->GetMaterialComponents())
	{
		
		matComp.pMatCompEqDirectXResource =
			m_pEffect->GetVariableByName(matComp.pMatCompDirectXVarName)->AsShaderResource();

		if (!matComp.pMatCompEqDirectXResource->IsValid())
			std::wcout << matComp.pMatCompDirectXVarName << L" is not valid \n";

		matComp.pMatCompEqDirectXResource->SetResource(matComp.pMatCompTexture->GetSRV());

	}

}

Material& BaseEffect::GetMaterial() const
{
	return *m_Material;
}
