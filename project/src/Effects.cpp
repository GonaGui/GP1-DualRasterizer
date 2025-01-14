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

}

void Effects::UpdateData(const float* worldProjViewMatrix, const float* worldMatrix, const float* cameraPos) const
{
	BaseEffect::UpdateData(worldProjViewMatrix, worldMatrix, cameraPos);
	m_pWorldMatrixVariable->SetMatrix(worldMatrix);
	m_pCameraPosition->SetFloatVector(cameraPos);
}

