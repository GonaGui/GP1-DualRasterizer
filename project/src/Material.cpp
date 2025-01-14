#include "Material.h"

#include <stdexcept>

#include "Texture.h"

Material::Material(const std::initializer_list<MatCompFormat>& materialInfo, ID3D11Device* directXDevice):m_MaterialComponents(materialInfo)
{
	for (const auto& matInfo : m_MaterialComponents)
	{
		matInfo.pMatCompTexture = new Texture();
		matInfo.pMatCompTexture->LoadFromFile(directXDevice, matInfo.pMatCompPath);

	}
}

Material::~Material()
{
	for (auto textures: m_MaterialComponents)
	{
		delete textures.pMatCompTexture;
	}
}

MatCompFormat Material::GetMaterialComponentByName(const std::string& directXVarName) const
{
	for (const auto& matInfo : m_MaterialComponents)
	{
		if (matInfo.pMatCompDirectXVarName == directXVarName)
		{
			return matInfo;
		}
	}

	throw std::runtime_error("Material component with name '" + directXVarName + "' not found.");
}

std::vector<MatCompFormat> Material::GetMaterialComponents()
{
	return m_MaterialComponents;
}
	
