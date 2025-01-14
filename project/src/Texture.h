#pragma once
#include <d3d11.h>
#include <string>


class Texture
{
public:
	~Texture();
	Texture();
	void LoadFromFile(ID3D11Device* pDevice, const std::string& path);
	ID3D11ShaderResourceView* GetSRV();
private:
	ID3D11Texture2D* m_pResource{};
	ID3D11ShaderResourceView* m_pSRV{};

};

