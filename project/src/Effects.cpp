#include "Effects.h"
#include <iostream>
#include <sstream>



Effects::Effects(ID3D11Device* pDevice, const std::wstring& assetFile): BaseEffect(pDevice, assetFile)
{
	m_Techniques.push_back("LinearTechnique");
	m_Techniques.push_back("PointTechnique");
	m_Techniques.push_back("AnisotropicTechnique");

	for (const auto techniques: m_Techniques)
	{
		m_pTechnique = m_pEffect->GetTechniqueByName(techniques);
		if (!m_pTechnique->IsValid())
			std::wcout << "The " << techniques << L" is not valid\n";
	}

	m_pWorldMatrixVariable = m_pEffect->GetVariableByName("gWorldMatrix")->AsMatrix();
	if (!m_pWorldMatrixVariable->IsValid())
		std::wcout << L"m_pWorldMatrixVariable not valid \n";

	m_pCameraPosition = m_pEffect->GetVariableByName("gCameraPosition")->AsVector();

	if (!m_pCameraPosition->IsValid())
		std::wcout << L"m_pCameraPosition not valid \n";
}

void Effects::UpdateData(const float* worldProjViewMatrix, const float* worldMatrix, const float* cameraPos) const
{
	BaseEffect::UpdateData(worldProjViewMatrix, worldMatrix, cameraPos);
	m_pWorldMatrixVariable->SetMatrix(worldMatrix);
	m_pCameraPosition->SetFloatVector(cameraPos);
}

