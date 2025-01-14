#include "pch.h"
#include "Renderer.h"
#include <array>
#include "AlphaEffect.h"
#include "Material.h"
#include "Utils.h"


namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//Initialize DirectX pipeline
		const HRESULT result = InitializeDirectX();
		if (result == S_OK)
		{
			m_IsInitialized = true;
			std::cout << "DirectX is initialized and ready!\n";
		}
		else
		{
			std::cout << "DirectX initialization failed!\n";
		}

		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

		m_pDepthBufferPixels = new float[m_Width * m_Height];

		m_AspectRatio = float(m_Width) / m_Height;


		//create some data for our mesh
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		m_AspectRatio = float(m_Width) / m_Height;

		m_MeshEffects["VehicleEffect"] = new Effects(m_pDevice, L"Resources/PosCol3D.fx");

		m_MeshEffects["FireEffect"] = new Effects(m_pDevice, L"Resources/PosColorAlpha.fx");

		Utils::ParseOBJ("Resources/vehicle.obj", vertices, indices,false);

		m_pMeshes.push_back(new Mesh
			{ m_pDevice,vertices,indices,m_MeshEffects["VehicleEffect"],
			{ MatCompFormat("gDiffuseMap", "Resources/vehicle_diffuse.png"),
			MatCompFormat("gNormalMap","Resources/vehicle_normal.png"),
			MatCompFormat("gSpecularMap","Resources/vehicle_specular.png"),
			MatCompFormat("gGlossinessMap","Resources/vehicle_gloss.png") } });

		Utils::ParseOBJ("Resources/fireFX.obj", vertices, indices, false);

		m_pMeshes.push_back(new Mesh
			{ m_pDevice,vertices,indices,m_MeshEffects["FireEffect"],
			{ MatCompFormat("gDiffuseMap", "Resources/fireFX_diffuse.png") } });


		m_DepthBuffer.resize(m_Width * m_Height, FLT_MAX);

		m_ClosestTriangle.resize(m_DepthBuffer.size());

		m_Camera.Initialize(45.f, { .0f,.0f,-50.f }, m_AspectRatio);

	}

	Renderer::~Renderer()
	{
		if (m_pDeviceContext) m_pDeviceContext->Release();
		if (m_pDevice) m_pDevice->Release();
		if (m_pSwapChain) m_pSwapChain->Release();
		if (m_pDepthStencilBuffer) m_pDepthStencilBuffer->Release();
		if (m_pDepthStencilView) m_pDepthStencilView->Release();
		if (m_pRenderTargetBuffer) m_pRenderTargetBuffer->Release();
		if (m_pRenderTargetView) m_pRenderTargetView->Release();

		for (auto mesh : m_pMeshes)
		{
			delete mesh;
		}
	}

	void Renderer::Update(Timer* pTimer)
	{
		m_Camera.Update(pTimer);
		if (m_IsRotating)
		{
			for (auto mesh : m_pMeshes)
			{
				mesh->UpdateWorldMatrixRotY(PI / 2, pTimer->GetTotal());
			}
			
		}
		
	}


	void Renderer::Render() const
	{
		if (!m_IsInitialized)
			return;

		// 1. CLEAR RTV & DSV
		constexpr float color[4] = { 0.f,0.f,0.3f,1.f };
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, color);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		// 1.5. GetWorldProjectionViewMatrix
		for (auto mesh : m_pMeshes)
		{
			Matrix WorldViewProjectionMatrix = mesh->GetWorldMatrix() * m_Camera.viewMatrix * m_Camera.projectionMatrix;
			Matrix WorldMatrix = mesh->GetWorldMatrix();

			// 2. SET PIPELINE + INVOKE DRAW CALLS (= RENDER)
			mesh->Render(m_pDeviceContext, WorldMatrix, WorldViewProjectionMatrix, m_Camera.GetOrigin());

		}
	

		// 3. PRESENT BACKBUFFER (SWAP)
		m_pSwapChain->Present(0, 0);

	}

	void Renderer::ToggleTechnique() const
	{
		for (auto mesh : m_pMeshes)
		{
			mesh->ToggleTechnique();
		}
	}

	void Renderer::ToggleRotation()
	{
		m_IsRotating = !m_IsRotating;
	}

	HRESULT Renderer::InitializeDirectX()
	{
		// 1. Create Device & DeviceContext

		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
		uint32_t createDeviceFlags = 0;

	#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
	#endif

		HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel,
			1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);

		if (FAILED(result))
		{
			std::cout << "failed to index Create Device" << std::endl;
			return result;
		}


		// 2. Create a Swapchain

		IDXGIFactory1* pDxgiFactory{};

		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pDxgiFactory));

		if (FAILED(result))
		{
			std::cout << "failed to index Create Factory" << std::endl;
			return result;
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;

		// Get the handle (HWND) from the SDL backbuffer
		SDL_SysWMinfo sysWMInfo{};
		SDL_GetVersion(&sysWMInfo.version);
		SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);
		swapChainDesc.OutputWindow = sysWMInfo.info.win.window;

		// Create SwapChain
		result = pDxgiFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
		if (FAILED(result))
		{
			std::cout << "failed to index Create Swap Chain" << std::endl;
			return result;
		}

		// 3. Create DepthStencil (DS) & DepthStencilView (DSV)
		// Resource
		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		// View
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilViewDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);

		if (FAILED(result))
		{
			std::cout << "failed to index Create Texture2D" << std::endl;
			return result;
		}

		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);

		if (FAILED(result))
		{
			std::cout << "failed to index Create Depth Stencil" << std::endl;
			return result;
		}

		// 4. Create RenderTarget (RT) & RenderTargetView (RTV)

		//Resources
		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));

		if (FAILED(result))
		{
			std::cout << "failed to index Create Render Target" << std::endl;
			return result;
		}

		// View
		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);

		if (FAILED(result))
		{
			std::cout << "failed to index Create Render View" << std::endl;
			return result;
		}

		// 5. Bind RTV & DSV to Output Merger Stage

		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		// 6. Set Viewport

		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(m_Width);
		viewport.Height = static_cast<float>(m_Height);
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		m_pDeviceContext->RSSetViewports(1, &viewport);

		result = S_OK;

		return result;
	}


	void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<VertexOut>& vertices_out, Mesh& CurrentMesh) const
	{
		vertices_out.resize(vertices_in.size());

		Matrix WorldViewProjectionMatrix = CurrentMesh.GetWorldMatrix() * m_Camera.viewMatrix * m_Camera.projectionMatrix;

		for (int idx{}; idx < vertices_in.size(); idx++)
		{
			vertices_out[idx].Position = WorldViewProjectionMatrix.TransformPoint(vertices_in[idx].Position.ToPoint4());

			vertices_out[idx].Position.x /= vertices_out[idx].Position.w;
			vertices_out[idx].Position.y /= vertices_out[idx].Position.w;
			vertices_out[idx].Position.z /= vertices_out[idx].Position.w;

			vertices_out[idx].UV = vertices_in[idx].UV;
			vertices_out[idx].Color = vertices_in[idx].Color;
			vertices_out[idx].Normal = CurrentMesh.GetWorldMatrix().TransformVector(vertices_in[idx].Normal).Normalized();
			vertices_out[idx].Tangent = CurrentMesh.GetWorldMatrix().TransformVector(vertices_in[idx].Tangent).Normalized();
			vertices_out[idx].WorldPosition = (CurrentMesh.GetWorldMatrix().TransformPoint(vertices_in[idx].Position)).ToVector4();
		}

	}

	void Renderer::ConvertToRasterSpace(std::array<Vector4, 3>& Triangle)
	{
		for (int idx{ 0 }; idx < 3; idx++)
		{
			Triangle[idx].x = ((Triangle[idx].x + 1) / 2) * m_Width;
			Triangle[idx].y = ((1 - Triangle[idx].y) / 2) * m_Height;
		}
	}

	void Renderer::ToggleIsNormalMapOn()
	{
		m_IsNormalMapOn = !m_IsNormalMapOn;
	}

	void Renderer::ToggleRenderingDepthBuffer()
	{
		m_IsRenderingDepthBuffer = !m_IsRenderingDepthBuffer;
	}

	bool Renderer::SaveBufferToImage() const
	{
		return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
	}

	bool Renderer::TriangleHitTest(uint32_t idx, Vector2 P, Vector3& a, Vector3& b, Vector3& c, std::array<Vector4, 3>& Triangle) const
	{

		a = Vector3(Triangle[0], Triangle[1]);
		b = Vector3(Triangle[1], Triangle[2]);
		c = Vector3(Triangle[2], Triangle[0]);

		Vector3 n{ Vector3::Cross(a,b).Normalized() };

		//loops through the 3 vertices of the current triangle
		for (int idx{}; idx < 3; idx++)
		{
			Vector3 e;
			if (idx == 0)
				e = a;
			else if (idx == 1)
				e = b;
			else
				e = c;

			Vector3 p{ Triangle[idx], Vector3(P.x,P.y,Triangle[idx].z) };

			if (Vector3::Dot(Vector3::Cross(e, p), n) < 0)
			{
				return false;
			}

		}

		return true;
	}

	void Renderer::CalculateBoundingBox(int& minX, int& minY, int& maxX, int& maxY, std::array<Vector4, 3>& Triangle) const
	{

		Vector3 v0 = Triangle[0];
		Vector3 v1 = Triangle[1];
		Vector3 v2 = Triangle[2];

		minX = static_cast<int>(std::floor(std::min({ v0.x, v1.x, v2.x })));
		minY = static_cast<int>(std::floor(std::min({ v0.y, v1.y, v2.y })));
		maxX = static_cast<int>(std::ceil(std::max({ v0.x, v1.x, v2.x })));
		maxY = static_cast<int>(std::ceil(std::max({ v0.y, v1.y, v2.y })));

		minX = std::clamp(minX, 0, m_Width - 1);
		minY = std::clamp(minY, 0, m_Height - 1);
		maxX = std::clamp(maxX, 0, m_Width - 1);
		maxY = std::clamp(maxY, 0, m_Height - 1);

	}

	bool Renderer::IsInFrustum(std::array<Vector4, 3>& Triangle) const
	{
		for (int idx{ 0 }; idx < 3; idx++)
		{
			if (Triangle[idx].x < -1 || Triangle[idx].x > 1)
			{
				return false;
			}

			if (Triangle[idx].y < -1 || Triangle[idx].y > 1)
			{
				return false;
			}

			if (Triangle[idx].z < 0 || Triangle[idx].z > 1)
			{
				return false;
			}

		}

		return true;
	}

	void Renderer::InterpolateValues(VertexOut& InterpolatedValues, std::array<Vector4, 3>& Triangle, Mesh& CurrentMesh, float WInterpolated, int idx, std::array<float, 3> Weights)
	{
		if (CurrentMesh.GetPrimitiveTopology() == PrimitiveTopology::TriangleList) idx *= 3;

		int indice0 = CurrentMesh.GetIndices()[idx];
		int indice1 = CurrentMesh.GetIndices()[idx + 1];
		int indice2 = CurrentMesh.GetIndices()[idx + 2];

		InterpolatedValues.UV = ((CurrentMesh.GetOutVertices()[indice0].UV / Triangle[0].w) * Weights[0] +
			(CurrentMesh.GetOutVertices()[indice1].UV / Triangle[1].w) * Weights[1] +
			(CurrentMesh.GetOutVertices()[indice2].UV / Triangle[2].w) * Weights[2]) * WInterpolated;

		InterpolatedValues.Normal = ((CurrentMesh.GetOutVertices()[indice0].Normal / Triangle[0].w) * Weights[0] +
			(CurrentMesh.GetOutVertices()[indice1].Normal / Triangle[1].w) * Weights[1] +
			(CurrentMesh.GetOutVertices()[indice2].Normal / Triangle[2].w) * Weights[2]) * WInterpolated;

		InterpolatedValues.Tangent = ((CurrentMesh.GetOutVertices()[indice0].Tangent / Triangle[0].w) * Weights[0] +
			(CurrentMesh.GetOutVertices()[indice1].Tangent / Triangle[1].w) * Weights[1] +
			(CurrentMesh.GetOutVertices()[indice2].Tangent / Triangle[2].w) * Weights[2]) * WInterpolated;

		InterpolatedValues.ViewDirection = ((CurrentMesh.GetOutVertices()[indice0].viewDirection / Triangle[0].w) * Weights[0] +
			(CurrentMesh.GetOutVertices()[indice1].viewDirection / Triangle[1].w) * Weights[1] +
			(CurrentMesh.GetOutVertices()[indice2].viewDirection / Triangle[2].w) * Weights[2]) * WInterpolated;

		InterpolatedValues.Position = ((CurrentMesh.GetOutVertices()[indice0].Position / Triangle[0].w) * Weights[0] +
			(CurrentMesh.GetOutVertices()[indice1].Position / Triangle[1].w) * Weights[1] +
			(CurrentMesh.GetOutVertices()[indice2].Position / Triangle[2].w) * Weights[2]) * WInterpolated;


	}

	ColorRGB Renderer::PixelShading(const VertexOut& v)
	{
		ColorRGB ambient{ .03f,.03f,.03f };
		Vector3 lightDirection = { .577f,-.577f,.577f };

		float kd = 7.f;
		ColorRGB cd = m_Texture->Sample(v.Uv); 

		// Grabs the Normal colors of the Normal map and then converts it to a usable format
		Vector3 NormalMap;

		if (m_IsNormalMapOn)
		{
			ColorRGB normalMapColor = m_NormalMap->Sample(v.Uv);
			Vector3 binormal = Vector3::Cross(v.Normal, v.Tangent);
			Matrix tangentSpaceAxis = Matrix{ v.Tangent, binormal, v.Normal, Vector3{} };
			NormalMap = { normalMapColor.r,normalMapColor.g,normalMapColor.b };
			NormalMap = 2.f * NormalMap - Vector3{ 1.f,1.f,1.f };
			NormalMap = tangentSpaceAxis.TransformVector(NormalMap);
		}

		else
		{
			NormalMap = v.Normal;
		}



		float observedArea = Vector3::Dot(NormalMap, -lightDirection);

		float shininess = 25.f;
		ColorRGB glossMapColor = m_GlossinessMap->Sample(v.Uv);
		ColorRGB specularMapColor = m_SpecularMap->Sample(v.Uv);
		specularMapColor *= shininess;
		Vector3 reflect = -lightDirection - 2 * NormalMap * observedArea;
		float cosa{ std::max(Vector3::Dot(reflect, v.ViewDirection), 0.f) };
		ColorRGB PhongSpecReflec = ColorRGB{ 1,1,1 } *glossMapColor.r * (std::pow(cosa, specularMapColor.r));


		if (observedArea <= 0) return ColorRGB{ 0,0,0 };

		switch (m_CurrentRenderMode)
		{
		case RenderMode::ObservedArea:
			return ColorRGB{ observedArea,observedArea,observedArea };
		case RenderMode::Combined:
			return (((kd * cd) / PI) + PhongSpecReflec + ambient) * observedArea;
		case RenderMode::Specular:
			return PhongSpecReflec;
		case RenderMode::Diffuse:
			return ((kd * cd) / PI);
		default:
			return ColorRGB{ 0,0,0 };
		}
	}

}
