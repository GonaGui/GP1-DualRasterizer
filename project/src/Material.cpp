#include "Material.h"
#include "Texture.h"

Material::Material(std::initializer_list<MatCompFormat>& materialInfo, ID3D11Device* directXDevice):m_MaterialComponents(materialInfo)
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

std::vector<MatCompFormat>& Material::GetMaterialComponents()
{
	return m_MaterialComponents;
}
