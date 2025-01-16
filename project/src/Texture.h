#pragma once
#include <d3d11.h>
#include <string>

#include "ColorRGBA.h"
#include "SDL_surface.h"


namespace dae
{
	struct Vector2;
}

class Texture
{
public:
	~Texture();
	Texture();
	void LoadFromFile(ID3D11Device* pDevice, const std::string& path);
	ID3D11ShaderResourceView* GetSRV();
	dae::ColorRGBA Sample(const dae::Vector2& uv) const;
private:
	Texture(SDL_Surface* pSurface);

	ID3D11Texture2D* m_pResource{};
	ID3D11ShaderResourceView* m_pSRV{};

	SDL_Surface* m_pSurface{ nullptr };
	uint32_t* m_pSurfacePixels{ nullptr };
};

