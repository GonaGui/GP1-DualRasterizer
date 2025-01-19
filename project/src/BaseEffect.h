#pragma once
#include <cstdint>
#include <vector>
#include "d3dx11effect.h"
#include "Texture.h"
struct MatCompFormat;
class Material;

class BaseEffect
{
public:
	BaseEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
	virtual ~BaseEffect();

	BaseEffect(const BaseEffect&) = delete;
	BaseEffect(BaseEffect&&) noexcept = default;
	BaseEffect& operator=(const BaseEffect&) = delete;
	BaseEffect& operator=(BaseEffect&&) noexcept = delete;

	static  ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
	ID3DX11Effect* GetEffect() const;
	ID3DX11EffectTechnique* GetTechnique() const;
	const char* GetCurrentTechnique() const;

	virtual void UpdateData(const float* worldProjViewMatrix, const float* worldMatrix, const float* cameraPos) const;
	void ToggleTechnique();
	virtual void SetMaterial(std::initializer_list<MatCompFormat>& materialComponent, ID3D11Device* directXDevice);
	Material& GetMaterial() const;

protected:

	ID3DX11Effect* m_pEffect{ nullptr };
	ID3DX11EffectTechnique* m_pTechnique{ nullptr };
	ID3DX11EffectMatrixVariable* m_pMatWorldViewProjVariable{};
	ID3DX11EffectMatrixVariable* m_pWorldMatrixVariable{};
	ID3DX11EffectVectorVariable* m_pCameraPosition{};
	Material* m_Material;
	std::vector<const char*> m_Techniques{};
	uint16_t m_CurrentTechnique{ 0 };
};
