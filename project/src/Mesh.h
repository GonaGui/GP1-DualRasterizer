#pragma once
#include <d3d11.h>
#include <vector>

#include "Camera.h"
#include "Matrix.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Effects.h"


struct Vertex
{
	dae::Vector3 Position{};
	dae::Vector3 Color{};
	dae::Vector2 UV{};
	dae::Vector3 Normal{};
	dae::Vector3 Tangent{};
	dae::Vector4 WorldPosition{};
};

struct VertexOut
{
	dae::Vector4 Position{};
	dae::Vector3 Color{};
	dae::Vector2 UV{};
	dae::Vector3 Normal{};
	dae::Vector3 Tangent{};
	dae::Vector4 WorldPosition{};
};

enum PrimitiveTopology
{
	TriangleList,
	TriangleStrip
};

enum CullModes
{
	FrontFaceCull,
	BackFaceCull,
	NoCull,
	CullModesEnd
};

class Mesh
{
public:

	Mesh(ID3D11Device* pDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, 
		BaseEffect* effect, std::initializer_list<MatCompFormat> materialComponents);

	~Mesh();

	Mesh(const Mesh&) = delete;
	Mesh(Mesh&&) noexcept = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh& operator=(Mesh&&) noexcept = delete;

	void Render(ID3D11DeviceContext* pDeviceContext, dae::Matrix& worldMatrix, dae::Matrix& worldProjViewMatrix, const float* cameraPos) const;
	void ToggleTechnique() const;
	PrimitiveTopology GetPrimitiveTopology();
	void SetPrimitiveTopology(PrimitiveTopology primitiveTopologyType);
	void UpdateWorldMatrixRotY(float yaw, float deltaSeconds);
	dae::Matrix GetWorldMatrix();
	void SetWorldMatrix(dae::Matrix newMatrix);
	MatCompFormat& GetMaterialComponentByName(std::string directXVarName) const;
	bool HasMaterialByComponentName(std::string directXVarName) const;
	std::vector<VertexOut>& GetOutVertices();
	std::vector<Vertex>& GetVertices();
	std::vector<uint32_t>& GetIndices();
	void ToggleCullMode();
	CullModes GetCurrentCullMode() const;

private:
	ID3D11InputLayout*		m_pInputLayout{ nullptr };
	uint32_t				m_NumIndices{};
	ID3D11Buffer*			m_pVertexBuffer{ nullptr };
	std::vector<Vertex>		m_Vertices{};
	std::vector<VertexOut>	m_VerticesOut{};
	ID3D11Buffer*			m_pIndexBuffer{ nullptr };
	std::vector<uint32_t>	m_Indices{};
	BaseEffect*				m_pEffect { nullptr };
	PrimitiveTopology       m_PrimitiveTopology{ TriangleList };
	CullModes               m_CurrentCullMode{NoCull};

	dae::Matrix				m_WorldMatrix{ };

	ID3D11RasterizerState* m_pCullingFront;
	ID3D11RasterizerState* m_pCullingBack;
	ID3D11RasterizerState* m_pCullingNone;
};
