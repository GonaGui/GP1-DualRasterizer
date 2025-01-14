#pragma once
#include <initializer_list>
#include <vector>
#include "d3dx11effect.h"

class Texture;

struct MatCompFormat
{
	const char* pMatCompDirectXVarName;
	const char* pMatCompPath;
	mutable Texture* pMatCompTexture;
	mutable ID3DX11EffectShaderResourceVariable* pMatCompEqDirectXResource;

	MatCompFormat(const char* matCompDirectXVarName, const char* matCompPath) :
		pMatCompDirectXVarName{matCompDirectXVarName}, pMatCompPath{matCompPath}, pMatCompTexture(nullptr),
		pMatCompEqDirectXResource(nullptr)
	{
		
	}

	MatCompFormat() :
		pMatCompDirectXVarName{ nullptr }, pMatCompPath{ nullptr }, pMatCompTexture(nullptr),
		pMatCompEqDirectXResource(nullptr)
	{
	}

};

class Material
{
public:
	Material(std::initializer_list<MatCompFormat>& materialInfo, ID3D11Device* directXDevice);
	~Material();
	std::vector<MatCompFormat>& GetMaterialComponents();
	

private:

	std::vector<MatCompFormat> m_MaterialComponents;

};

