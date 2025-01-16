#pragma once
#include <unordered_map>

#include "Camera.h"
#include "Effects.h"
#include "Mesh.h"

#include <stdlib.h>


struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render() const;

		bool SaveBufferToImage() const;
		static bool TriangleHitTest(uint32_t idx, Vector2 P, Vector3& a, Vector3& b, Vector3& c, const std::array<Vector4, 3>& triangle);
		void CalculateBoundingBox(int& minX, int& minY, int& maxX, int& maxY, const std::array<Vector4, 3>& triangle) const;
		static bool IsInFrustum(std::array<Vector4, 3>& triangle);
		static void InterpolateValues(VertexOut& interpolatedValues, const std::array<Vector4, 3>& triangle, Mesh& currentMesh, float wInterpolated, int idx, std::array<float, 3> weights);
		ColorRGBA PixelShading(const VertexOut& v, const Mesh* currentMesh) const;

		void VertexTransformationFunction(const std::vector<Vertex>& verticesIn, std::vector<VertexOut>& verticesOut, Mesh& currentMesh) const;
		void ConvertToRasterSpace(std::array<Vector4, 3>& vertices) const;
		void ToggleOptions(SDL_Scancode keyScancode);
		static std::string OnOrOff(bool trueOrFalse);

	private:
		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		enum RenderModes 
		{
			ObservedArea,
			Diffuse,
			Specular,
			Combined,
			RenderModesEnd
		};

		RenderModes m_CurrentRenderMode{ Combined };

		bool m_IsNormalMapOn{ true };
		bool m_IsRenderingDepthBuffer{};
		bool m_IsBoundingBoxVisualisation{};
		bool m_UniformColor{};
		bool m_RenderFireMesh{true};

		std::vector<float> m_DepthBuffer{};
		std::vector<int> m_ClosestTriangle{};

		bool m_IsInitialized{ false };

		ID3D11Device* m_pDevice{nullptr};
		ID3D11DeviceContext* m_pDeviceContext{ nullptr };
		IDXGISwapChain* m_pSwapChain{ nullptr };
		ID3D11Texture2D* m_pDepthStencilBuffer{ nullptr };
		ID3D11DepthStencilView* m_pDepthStencilView{ nullptr };
		ID3D11Resource* m_pRenderTargetBuffer{ nullptr };
		ID3D11RenderTargetView* m_pRenderTargetView{ nullptr };

		std::vector <Mesh*> m_pMeshes;
		std::unordered_map <std::string, BaseEffect*> m_MeshEffects;
		std::unordered_map<std::string, std::vector<MatCompFormat>> m_MeshTextures;

		Camera m_Camera;
		float m_AspectRatio{};
		Matrix	m_WorldMatrix{};

		bool m_IsRotating{};
		bool m_IsSoftwareRasterizer{};



		//DIRECTX
		HRESULT InitializeDirectX();
		//...



	};

}
