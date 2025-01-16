#include "Texture.h"
#include <iostream>
#include <SDL_image.h>

#include "Vector2.h"

Texture::~Texture()
{
 	m_pResource->Release();
	m_pSRV->Release();
}

Texture::Texture()
{
	if (m_pSurface)
	{
		SDL_FreeSurface(m_pSurface);
		m_pSurface = nullptr;
	}
}

void Texture::LoadFromFile(ID3D11Device* pDevice, const std::string& path)
{
	SDL_Surface* surface = IMG_Load(path.c_str());

	m_pSurface = surface;
	m_pSurfacePixels = static_cast<uint32_t*>(surface->pixels);

	SDL_Surface* pSurface = IMG_Load(path.c_str());
	if (!pSurface)
	{
		std::cout << "Failed to load image: " << path << " SDL_Error: " << SDL_GetError() << std::endl;
		return;
	}

	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = pSurface->w;
	desc.Height = pSurface->h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = pSurface->pixels;
	initData.SysMemPitch = static_cast<UINT>(pSurface->pitch);
	initData.SysMemSlicePitch = static_cast<UINT>(pSurface->h * pSurface->pitch);

	HRESULT hr = pDevice->CreateTexture2D(&desc, &initData, &m_pResource);

	if (FAILED(hr))
	{
		std::cout << "failed to Create Texture2D" << std::endl;
		return ;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	hr = pDevice->CreateShaderResourceView(m_pResource, &SRVDesc, &m_pSRV);
	if (FAILED(hr))
	{
		std::cout << "failed to Create Shader Resource View" << std::endl;
		return ;
	}

}

ID3D11ShaderResourceView* Texture::GetSRV()
{
	return m_pSRV;
}

dae::ColorRGBA Texture::Sample(const dae::Vector2& uv) const
{
	float x = uv.x - std::floor(uv.x);
	float y = uv.y - std::floor(uv.y);
	x = uint32_t(x * m_pSurface->w);
	y = uint32_t(y * m_pSurface->h);


	uint8_t r, g, b, a;
	Uint8* pPixelAddress = (Uint8*)m_pSurfacePixels + uint32_t(y) * m_pSurface->pitch + uint32_t(x) * m_pSurface->format->BytesPerPixel;

	SDL_GetRGBA(*(Uint32*)pPixelAddress, m_pSurface->format, &r, &g, &b,&a);

	return {
		r / 255.f ,
		g / 255.f,
		b / 255.f,
		a / 255.f
	};
}

Texture::Texture(SDL_Surface* pSurface) :
	m_pSurface{ pSurface },
	m_pSurfacePixels{ static_cast<uint32_t*>(pSurface->pixels) }
{
}
