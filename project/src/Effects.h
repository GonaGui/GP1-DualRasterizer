#pragma once
#include "BaseEffect.h"
#include <string>



class Effects: public BaseEffect
{
public:
	Effects(ID3D11Device* pDevice, const std::wstring& assetFile) ;

	void UpdateData(const float* worldProjViewMatrix, const float* worldMatrix, const float* cameraPos) const override;

private:
};
