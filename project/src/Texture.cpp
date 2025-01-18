#include "Texture.h"
#include <iostream>
#include <SDL_image.h>

#include "Mesh.h"
#include "Vector2.h"
#undef min

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
	x = x * m_pSurface->w;
	y = y * m_pSurface->h;


	uint8_t r, g, b, a;



	//POINT SAMPLING
	{
		Uint8* pPixelAddress = (Uint8*)m_pSurfacePixels + uint32_t(y) * m_pSurface->pitch + uint32_t(x) * m_pSurface->format->BytesPerPixel;

		SDL_GetRGBA(*(Uint32*)pPixelAddress, m_pSurface->format, &r, &g, &b,&a);

		return {
			r / 255.f ,
			g / 255.f,
			b / 255.f,
			a / 255.f
		};
	}

}

dae::ColorRGBA Texture::Sample(const dae::Vector2& uv, const Mesh* currentMesh) const
{
	float x = uv.x - std::floor(uv.x);
	float y = uv.y - std::floor(uv.y);
	x = x * m_pSurface->w;
	y = y * m_pSurface->h;


	uint8_t r, g, b, a;

	Uint8* pPixelAddress = (Uint8*)m_pSurfacePixels + uint32_t(y) * m_pSurface->pitch + uint32_t(x) * m_pSurface->format->BytesPerPixel;

	if (currentMesh->GetCurrentTechnique() == "LinearTechnique")
	{
		//Linear Sampling
		float x1 = x - 0.5f;
		float x2 = x + 0.5f;
		float y1 = y - 0.5f;
		float y2 = y + 0.5f;

		// Find the nearest integer pixel positions
		int xPxl = std::floor(x);
		int yPxl = std::floor(y);

		if (x - xPxl > 0.5f) xPxl += 1; // Adjust x if in the right quadrant
		if (y - yPxl > 0.5f) yPxl += 1; // Adjust y if in the bottom quadrant

		// Calculate addresses for the 4 surrounding texels
		Uint8* topLeftPixel = (Uint8*)m_pSurfacePixels + uint32_t(y1) * m_pSurface->pitch + uint32_t(x1) * m_pSurface->format->BytesPerPixel;
		Uint8* topRightPixel = (Uint8*)m_pSurfacePixels + uint32_t(y1) * m_pSurface->pitch + uint32_t(x2) * m_pSurface->format->BytesPerPixel;
		Uint8* bottomLeftPixel = (Uint8*)m_pSurfacePixels + uint32_t(y2) * m_pSurface->pitch + uint32_t(x1) * m_pSurface->format->BytesPerPixel;
		Uint8* bottomRightPixel = (Uint8*)m_pSurfacePixels + uint32_t(y2) * m_pSurface->pitch + uint32_t(x2) * m_pSurface->format->BytesPerPixel;

		dae::ColorRGBA finalColor{};	
		uint8_t r, g, b, a;

		// Top-left
		SDL_GetRGBA(*(Uint32*)topLeftPixel, m_pSurface->format, &r, &g, &b, &a);
		finalColor += dae::ColorRGBA(r, g, b, a) * ((xPxl - x1) * (yPxl - y1));

		// Top-right
		SDL_GetRGBA(*(Uint32*)topRightPixel, m_pSurface->format, &r, &g, &b, &a);
		finalColor += dae::ColorRGBA(r, g, b, a) * ((x2 - xPxl) * (yPxl - y1));

		// Bottom-left
		SDL_GetRGBA(*(Uint32*)bottomLeftPixel, m_pSurface->format, &r, &g, &b, &a);
		finalColor += dae::ColorRGBA(r, g, b, a) * ((xPxl - x1) * (y2 - yPxl));

		// Bottom-right
		SDL_GetRGBA(*(Uint32*)bottomRightPixel, m_pSurface->format, &r, &g, &b, &a);
		finalColor += dae::ColorRGBA(r, g, b, a) * ((x2 - xPxl) * (y2 - yPxl));

		SDL_GetRGBA(*(Uint32*)pPixelAddress, m_pSurface->format, &r, &g, &b, &a);
	
		finalColor.a = a / 255.f;

		return finalColor / 255.f;
	}

	if (currentMesh->GetCurrentTechnique() == "AnisotropicTechnique")
	{
	}

	

	SDL_GetRGBA(*(Uint32*)pPixelAddress, m_pSurface->format, &r, &g, &b, &a);

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
