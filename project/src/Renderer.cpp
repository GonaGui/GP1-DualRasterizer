#include "pch.h"
#include "Renderer.h"
#include <array>
#include <numeric>

#include "AlphaEffect.h"
#include "Material.h"
#include "Utils.h"

#define ESC "\033["
#define YELLOW_TXT "33"
#define GREEN_TXT "32"
#define PURPLE_TXT "35"
#define RESET "\033[m"


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

		m_MeshEffects["FireEffect"] = new AlphaEffect(m_pDevice, L"Resources/PosColorAlpha.fx");

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
	 		{ MatCompFormat("gDiffuseMap", "Resources/fireFX_diffuse.png") },
	 			true });
		
		for (auto& mesh: m_pMeshes )
		{
			mesh->SetWorldMatrix(Matrix::CreateTranslation(0,0,50));
		}
		m_DepthBuffer.resize(m_Width * m_Height, FLT_MAX);

		m_ClosestTriangle.resize(m_DepthBuffer.size());

		m_Camera.Initialize(45.f, { .0f,.0f,0.f }, m_AspectRatio);

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

		delete[] m_pDepthBufferPixels;

		for (const auto mesh : m_pMeshes)
		{
			delete mesh;
		}
	}

	void Renderer::Update(Timer* pTimer)
	{
		m_Camera.Update(pTimer); 
		if (m_IsRotating)
		{
			for (const auto mesh : m_pMeshes)
			{
				mesh->UpdateWorldMatrixRotY(PI / 2, pTimer->GetElapsed());
			}
			
		}
		
	}


	

	void Renderer::Render() const
	{
		ColorRGBA backgroundColor{ .1f, .1f, .1f };

		if (m_IsSoftwareRasterizer)
		{
			//@START
			//Lock BackBuffer
			if (m_UniformColor == false)
			{
				backgroundColor = { 0.39,0.39 ,0.39 };
			}
	
			SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(backgroundColor.r * 255),
				static_cast<uint8_t>(backgroundColor.g * 255),
				static_cast<uint8_t>(backgroundColor.b * 255)));
			SDL_LockSurface(m_pBackBuffer);

			for (int idx{}; idx < m_Width * m_Height; idx++)
			{
				m_pDepthBufferPixels[idx] = FLT_MAX;
			}

			for (auto currentMesh: m_pMeshes)
			{
				if (currentMesh == m_pMeshes[1] && m_RenderFireMesh == false) continue;


				auto& verticesOut = currentMesh->GetOutVertices();
				auto&  indices = currentMesh->GetIndices();
				VertexTransformationFunction(currentMesh->GetVertices(),
					verticesOut, *currentMesh);

				int nrOfTriangles;

				if (currentMesh->GetPrimitiveTopology() == TriangleList)
				{
					nrOfTriangles = indices.size() / 3;
				}

				else
				{
					nrOfTriangles = indices.size() - 2;
				}

				//RENDER LOGIC
				//loops through all triangles
				for (int idx{ 0 }; idx < nrOfTriangles; idx++)
				{
					uint32_t indice0;
					uint32_t indice1;
					uint32_t indice2;

					indice0 = indices[idx * 3];
					indice1 = indices[idx * 3 + 1];
					indice2 = indices[idx * 3 + 2];

					if (currentMesh->GetPrimitiveTopology() == TriangleStrip)
					{
						indice0 = indices[idx];
						indice1 = indices[idx + 1];
						indice2 = indices[idx + 2];
					}

					int minX;
					int minY;
					int maxX;
					int maxY;

					std::array<Vector4, 3> triangle{ verticesOut[indice0].Position,
													 verticesOut[indice1].Position,
													 verticesOut[indice2].Position };

					if (!IsInFrustum(triangle)) continue;

					ConvertToRasterSpace(triangle);
					CalculateBoundingBox(minX, minY, maxX, maxY, triangle);

					Vector3 a {triangle[0], triangle[1]};
					Vector3 b {triangle[1], triangle[2]};
					Vector3 c {triangle[2], triangle[0]};

					const Vector3 n{ Vector3::Cross(a,b).Normalized() };

					for (int py{ minY }, px; py < maxY; ++py) 
					for (px = minX; px < maxX; ++px)
					{
						const int depthBufferIndex{ px + (py * m_Width) };

						ColorRGBA finalColor{ 0, 0, 0 };
						Vector2 P{ px + 0.5f,py + 0.5f };

						if (m_IsBoundingBoxVisualisation) 
						{
							finalColor = ColorRGBA(1, 1, 1);

							if (px == maxX - 1) finalColor = ColorRGBA(1, 0, 0);
							if (px == minX) finalColor = ColorRGBA(1, 0, 0);
							if (py == maxY - 1) finalColor = ColorRGBA(1, 0, 0);
							if (py == minY) finalColor = ColorRGBA(1, 0, 0);


							m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
								static_cast<uint8_t>(finalColor.r * 255),
								static_cast<uint8_t>(finalColor.g * 255),
								static_cast<uint8_t>(finalColor.b * 255));

							continue;
						}

						float triangleArea = Vector2::Cross(a.GetXY(), b.GetXY());

						if (triangleArea < 0 && currentMesh->GetCurrentCullMode() == BackFaceCull)
							continue;

						if (triangleArea > 0 && currentMesh->GetCurrentCullMode() == FrontFaceCull)
							continue;

						std::array<float, 3> weights{
							Vector2::Cross(Vector2(P, triangle[1].GetXY()), b.GetXY()),
							Vector2::Cross(Vector2(P, triangle[2].GetXY()), c.GetXY()),
							Vector2::Cross(Vector2(P, triangle[0].GetXY()), a.GetXY())
						};

						weights[0] /= triangleArea;
						weights[1] /= triangleArea;
						weights[2] /= triangleArea;

						if( !(weights[0] > 0 && weights[1] > 0 && weights[2] > 0) || (weights[0] < 0 && weights[1] < 0 && weights[2] < 0)) continue;


						float ZInterpolated = 1.f / (weights[0] / triangle[0].z +
							weights[1] / triangle[1].z +
							weights[2] / triangle[2].z);

						float WInterpolated = 1.f / (weights[0] / triangle[0].w +
							weights[1] / triangle[1].w +
							weights[2] / triangle[2].w);

						if (ZInterpolated >= m_pDepthBufferPixels[depthBufferIndex] || ZInterpolated <= 0 || ZInterpolated >= 1)
							continue;

						VertexOut interpolatedValues;
						InterpolateValues(interpolatedValues, triangle, *currentMesh, WInterpolated, idx, weights);

						interpolatedValues.Position.z = ZInterpolated;
						interpolatedValues.Position.w = WInterpolated;


						if (!currentMesh->GetUsesTransparency()) m_pDepthBufferPixels[depthBufferIndex] = ZInterpolated;

						if (m_IsRenderingDepthBuffer) 
						{
							Utils::Remap(ZInterpolated, 0.985f, 1);
							finalColor = ColorRGBA(ZInterpolated, ZInterpolated, ZInterpolated);
						}
						else 
							finalColor = PixelShading(interpolatedValues, currentMesh);

						uint8_t r, g, b;
						SDL_GetRGB(m_pBackBufferPixels[depthBufferIndex], m_pBackBuffer->format, &r, &g, &b);

						finalColor *= finalColor.a;

						finalColor += ColorRGBA(r/255.f, g / 255.f, b / 255.f) * (1 - finalColor.a);

						// Update Color in Buffer
						finalColor.MaxToOne();
						m_pBackBufferPixels[depthBufferIndex] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));

					}
					


				}
			}

			//@END
			//Update SDL Surface
			SDL_UnlockSurface(m_pBackBuffer);
			SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
			SDL_UpdateWindowSurface(m_pWindow);
		}

		else
		{
			if (m_UniformColor == false)
			{
				backgroundColor = { .39f, .59f, .93f };
			}

			if (!m_IsInitialized)
				return;

			// 1. CLEAR RTV & DSV
			float color[4] = { backgroundColor.r, backgroundColor.g, backgroundColor.b };
			m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, color);
			m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

			// 1.5. GetWorldProjectionViewMatrix
			for (auto mesh : m_pMeshes)
			{
				if (mesh == m_pMeshes[1] && m_RenderFireMesh == false) continue;

		
				Matrix worldViewProjectionMatrix = mesh->GetWorldMatrix() * m_Camera.viewMatrix * m_Camera.projectionMatrix;
				Matrix worldMatrix = mesh->GetWorldMatrix();

				// 2. SET PIPELINE + INVOKE DRAW CALLS (= RENDER)
				mesh->Render(m_pDeviceContext, worldMatrix, worldViewProjectionMatrix, m_Camera.GetOrigin());
			
				

			}


			// 3. PRESENT BACKBUFFER (SWAP)
			m_pSwapChain->Present(0, 0);
		}
		

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

	void Renderer::VertexTransformationFunction(const std::vector<Vertex>& verticesIn, std::vector<VertexOut>& verticesOut, Mesh& currentMesh) const
	{
		verticesOut.resize(verticesIn.size());

		const Matrix worldMatrix = currentMesh.GetWorldMatrix();

		const Matrix worldViewProjectionMatrix = currentMesh.GetWorldMatrix() * m_Camera.viewMatrix * m_Camera.projectionMatrix;

		for (int idx{}; idx < verticesIn.size(); idx++)
		{
			verticesOut[idx].Position = worldViewProjectionMatrix.TransformPoint(verticesIn[idx].Position.ToPoint4());

			verticesOut[idx].Position.x /= verticesOut[idx].Position.w;
			verticesOut[idx].Position.y /= verticesOut[idx].Position.w;
			verticesOut[idx].Position.z /= verticesOut[idx].Position.w;

			verticesOut[idx].UV = verticesIn[idx].UV;
			verticesOut[idx].Color = verticesIn[idx].Color;
			verticesOut[idx].Normal = worldMatrix.TransformVector(verticesIn[idx].Normal).Normalized();
			verticesOut[idx].Tangent = worldMatrix.TransformVector(verticesIn[idx].Tangent).Normalized();
			verticesOut[idx].WorldPosition = (worldMatrix.TransformPoint(verticesIn[idx].Position)).ToVector4();
		}

	}

	void Renderer::ConvertToRasterSpace(std::array<Vector4, 3>& triangle) const
	{
		for (int idx{ 0 }; idx < 3; idx++)
		{
			triangle[idx].x = ((triangle[idx].x + 1) / 2) * m_Width;
			triangle[idx].y = ((1 - triangle[idx].y) / 2) * m_Height;
		}
	}

	void Renderer::ToggleOptions(const SDL_Scancode keyScancode)
	{
		if (keyScancode == SDL_SCANCODE_F1)
		{
			m_IsSoftwareRasterizer = !m_IsSoftwareRasterizer;

			std::string hardwareOrSoftware {};
			if (m_IsSoftwareRasterizer) hardwareOrSoftware = "SOFTWARE";

			else hardwareOrSoftware = "HARDWARE";
			
			std::cout << ESC << YELLOW_TXT << "m" << "(SHARED) " << "Rasterizer mode is " << hardwareOrSoftware << RESET << "\n\n";
		}
			

		if (keyScancode == SDL_SCANCODE_F2)
		{
			m_IsRotating = !m_IsRotating;

			std::cout << ESC << YELLOW_TXT << "m" << "(SHARED) " << "Mesh rotation is" << OnOrOff(m_IsRotating) << RESET << "\n\n";
		}
			

		if (keyScancode == SDL_SCANCODE_F3)
		{
			m_RenderFireMesh = !m_RenderFireMesh;
			std::cout << ESC << YELLOW_TXT << "m" << "(SHARED) " << "Fire effect is" << OnOrOff(m_RenderFireMesh) << RESET << "\n\n";
		}
			

		if (keyScancode == SDL_SCANCODE_F4)
		{
			for (const auto mesh : m_pMeshes)
			{
				mesh->ToggleTechnique();
			}


			std::cout << ESC << GREEN_TXT << "m" << "(HARDWARE) " <<"Current Technique is " << m_pMeshes[0]->GetCurrentTechnique() << RESET << "\n\n";
		}

		if (keyScancode == SDL_SCANCODE_F5)
		{
			if (!m_IsSoftwareRasterizer) return;

			m_CurrentRenderMode = static_cast<RenderModes>((m_CurrentRenderMode + 1) % static_cast<int>(RenderModesEnd));

			std::cout << ESC << PURPLE_TXT << "m" << "(SOFTWARE) " << "Current Render is " << GetCurrentRenderModeName() << RESET << "\n\n";
		}
		if (keyScancode == SDL_SCANCODE_F6)
		{
			if (!m_IsSoftwareRasterizer) return;
			m_IsNormalMapOn = !m_IsNormalMapOn;
			std::cout << ESC << PURPLE_TXT << "m" << "(SOFTWARE) " << "Normal map is" << OnOrOff(m_IsNormalMapOn) << RESET << "\n\n";
		}
			

		if (keyScancode == SDL_SCANCODE_F7)
		{
			if (!m_IsSoftwareRasterizer) return;
			m_IsRenderingDepthBuffer = !m_IsRenderingDepthBuffer;
			std::cout << ESC << PURPLE_TXT << "m" << "(SOFTWARE) " << "Rendering Depth Buffer map is" << OnOrOff(m_IsRenderingDepthBuffer) << RESET << "\n\n";
		}
			

		if (keyScancode == SDL_SCANCODE_F8)
		{
			if (!m_IsSoftwareRasterizer) return;
			m_IsBoundingBoxVisualisation = !m_IsBoundingBoxVisualisation;
			std::cout << ESC << PURPLE_TXT << "m" << "(SOFTWARE) " << "Bounding Box Visualisation is" << OnOrOff(m_IsBoundingBoxVisualisation) << RESET << "\n\n";
		}
			

		if (keyScancode == SDL_SCANCODE_F9)
		{
			for (const auto mesh : m_pMeshes)
			{
				mesh->ToggleCullMode();
			}

			std::cout << ESC << YELLOW_TXT << "m" << "(SHARED) " << "Current CullMode is " << m_pMeshes[0]->GetCurrentCullModeName() << RESET << "\n\n";
		}

		if (keyScancode == SDL_SCANCODE_F10)
		{
			m_UniformColor = !m_UniformColor;
			std::cout << ESC << YELLOW_TXT << "m" << "(SHARED) " << "Uniform Color is" << OnOrOff(m_UniformColor) << RESET << "\n\n";
		}
			
	}

	std::string Renderer::GetCurrentRenderModeName() const
	{
		switch (m_CurrentRenderMode)
		{
		case ObservedArea:
			return "Observed Area Render Mode";
			break;
		case Diffuse:
			return "Diffuse Render Mode";
			break;
		case Specular:
			return "Specular Render Mode";
			break;
		case Combined:
			return "Combined Render Mode";
			break;
		default: 
			return "Error no render mode";
		}

	}

	std::string Renderer::OnOrOff(bool trueOrFalse)
	{
		if (trueOrFalse) return std::string(" ON ");

		return std::string(" OFF ");
		
	}

	bool Renderer::SaveBufferToImage() const
	{
		return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
	}

	void Renderer::CalculateBoundingBox(int& minX, int& minY, int& maxX, int& maxY, const std::array<Vector4, 3>& triangle) const
	{

		Vector3 v0 = triangle[0];
		Vector3 v1 = triangle[1];
		Vector3 v2 = triangle[2];

		minX = static_cast<int>(std::floor(std::min({ v0.x, v1.x, v2.x })));
		minY = static_cast<int>(std::floor(std::min({ v0.y, v1.y, v2.y })));
		maxX = static_cast<int>(std::ceil(std::max({ v0.x, v1.x, v2.x })));
		maxY = static_cast<int>(std::ceil(std::max({ v0.y, v1.y, v2.y })));

		minX = std::clamp(minX, 0, m_Width - 1);
		minY = std::clamp(minY, 0, m_Height - 1);
		maxX = std::clamp(maxX, 0, m_Width - 1);
		maxY = std::clamp(maxY, 0, m_Height - 1);

	}

	bool Renderer::IsInFrustum(std::array<Vector4, 3>& triangle)
	{
		for (int idx{ 0 }; idx < 3; idx++)
		{
			if (triangle[idx].x < -1 || triangle[idx].x > 1)
			{
				return false;
			}

			if (triangle[idx].y < -1 || triangle[idx].y > 1)
			{
				return false;
			}

			if (triangle[idx].z < 0 || triangle[idx].z > 1)
			{
				return false;
			}

		}

		return true;
	}

	void Renderer::InterpolateValues(VertexOut& interpolatedValues, const std::array<Vector4, 3>& triangle, 
		Mesh& currentMesh, const float wInterpolated, int idx, const std::array<float, 3> weights)
	{
		const auto& indices = currentMesh.GetIndices();
		const auto& outVertices = currentMesh.GetOutVertices();
		if (currentMesh.GetPrimitiveTopology() == PrimitiveTopology::TriangleList) idx *= 3;

		const int indice0 = indices[idx];
		const int indice1 = indices[idx + 1];
		const int indice2 = indices[idx + 2];

		interpolatedValues.UV = ((outVertices[indice0].UV / triangle[0].w) * weights[0] +
			(outVertices[indice1].UV / triangle[1].w) * weights[1] +
			(outVertices[indice2].UV / triangle[2].w) * weights[2]) * wInterpolated;

		interpolatedValues.Normal = ((outVertices[indice0].Normal / triangle[0].w) * weights[0] +
			(outVertices[indice1].Normal / triangle[1].w) * weights[1] +
			(outVertices[indice2].Normal / triangle[2].w) * weights[2]) * wInterpolated;

		interpolatedValues.Tangent = ((outVertices[indice0].Tangent / triangle[0].w) * weights[0] +
			(outVertices[indice1].Tangent / triangle[1].w) * weights[1] +
			(outVertices[indice2].Tangent / triangle[2].w) * weights[2]) * wInterpolated;

		interpolatedValues.WorldPosition = ((outVertices[indice0].WorldPosition / triangle[0].w) * weights[0] +
			(outVertices[indice1].WorldPosition / triangle[1].w) * weights[1] +
			(outVertices[indice2].WorldPosition / triangle[2].w) * weights[2]) * wInterpolated;

		interpolatedValues.Position = ((outVertices[indice0].Position / triangle[0].w) * weights[0] +
			(outVertices[indice1].Position / triangle[1].w) * weights[1] +
			(outVertices[indice2].Position / triangle[2].w) * weights[2]) * wInterpolated;


	}

	ColorRGBA Renderer::PixelShading(const VertexOut& v, const Mesh* currentMesh) const
	{
		constexpr ColorRGBA ambient{ .025f,.025f,.025f };
		const Vector3 lightDirection = { .577f,-.577f,.577f };

		constexpr float kd = 7.f;

		const ColorRGBA cd = currentMesh->GetMaterialComponentByName(m_DiffuseMapString).pMatCompTexture->Sample(v.UV,currentMesh);

		// Grabs the Normal colors of the Normal map and then converts it to a usable format
		Vector3 normalMap { v.Normal};

		if (m_IsNormalMapOn)
		{
			if (currentMesh->HasMaterialByComponentName(m_NormalMapString))
			{
				ColorRGBA normalMapColor = currentMesh->GetMaterialComponentByName(m_NormalMapString).pMatCompTexture->Sample(v.UV, currentMesh);
				const auto tangent = v.Tangent.Normalized();
				const auto normal = v.Normal.Normalized();
				const Vector3 binormal = Vector3::Cross(normal, tangent);
				const Matrix tangentSpaceAxis = Matrix{ tangent, binormal, normal, Vector3{} };
				normalMap = { normalMapColor.r,normalMapColor.g,normalMapColor.b };
				normalMap = 2.f * normalMap - Vector3{ 1.f,1.f,1.f };
				normalMap = tangentSpaceAxis.TransformVector(normalMap);
			}

		}

		const float observedArea = Vector3::Dot(normalMap, -lightDirection);
		ColorRGBA phongSpecReflect{};


		if (currentMesh->HasMaterialByComponentName(m_SpecularMapString) && 
			currentMesh->HasMaterialByComponentName(m_GlossinessMapString))
		{
			const ColorRGBA specularMapColor = currentMesh->GetMaterialComponentByName(m_SpecularMapString).pMatCompTexture->Sample(v.UV, currentMesh);
			constexpr float shininess = 25.f;

			const ColorRGBA glossMapColor = currentMesh->GetMaterialComponentByName(m_GlossinessMapString).pMatCompTexture->Sample(v.UV,currentMesh);


			const Vector3 reflect = Vector3::Reflect(-lightDirection, normalMap);
			const Vector3 invViewDirection = (m_Camera.origin - v.WorldPosition.GetXYZ()).Normalized();
			const float cosa{ std::max(Vector3::Dot(reflect, -invViewDirection), 0.f) };

			phongSpecReflect = ColorRGBA{ 1,1,1 } *specularMapColor.r * (std::pow(cosa, glossMapColor.r * shininess));
		}


		if (currentMesh->GetUsesTransparency()) return cd; // if mesh has trasnparency just return pixel color

		if (observedArea < 0) return ColorRGBA{ 0,0,0,1 };
		

		switch (m_CurrentRenderMode)
		{
		case ObservedArea:
			return ColorRGBA{ observedArea,observedArea,observedArea };
		case Combined:
			return (((cd * kd) / PI) + phongSpecReflect + ambient) * observedArea;
		case Specular:
			return phongSpecReflect;
		case Diffuse:
			return ((cd * kd) / PI);
		default:
			return ColorRGBA{ 0,0,0 };
		}
	}

}
