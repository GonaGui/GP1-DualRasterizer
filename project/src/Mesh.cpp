#include "Mesh.h"
#include <cassert>
#include <cstdint>
#include <d3d11.h>
#include <iostream>

#include "Camera.h"
#include "SDL_system.h"

#include "d3dx11effect.h"
#include "Material.h"


Mesh::Mesh(ID3D11Device* pDevice, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, 
           BaseEffect* effect, std::initializer_list<MatCompFormat> materialComponents)
{
	m_Vertices = vertices;
	m_Indices = indices;

	m_pEffect = effect;

	//Create Vertex Layout
	static constexpr uint32_t numElements{ 6 };
	D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

	vertexDesc[0].SemanticName = "POSITION";
	vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[0].AlignedByteOffset = 0;
	vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[1].SemanticName = "WORLD";
	vertexDesc[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; 
	vertexDesc[1].AlignedByteOffset = offsetof(Vertex, WorldPosition); // Adjust to offset the correct position in the vertex
	vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[2].SemanticName = "NORMAL";
	vertexDesc[2].Format = DXGI_FORMAT_R32G32B32_FLOAT; // Use R32G32 for 2D texture coordinates
	vertexDesc[2].AlignedByteOffset = offsetof(Vertex, Normal); // Adjust to offset the correct position in the vertex
	vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[3].SemanticName = "TANGENT";
	vertexDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[3].AlignedByteOffset = offsetof(Vertex, Tangent);
	vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[4].SemanticName = "TEXCOORD";
	vertexDesc[4].Format = DXGI_FORMAT_R32G32_FLOAT; // Use R32G32 for 2D texture coordinates
	vertexDesc[4].AlignedByteOffset = offsetof(Vertex, UV); // Adjust to offset the correct position in the vertex
	vertexDesc[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[5].SemanticName = "COLOR";
	vertexDesc[5].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[5].AlignedByteOffset = offsetof(Vertex,Color);
	vertexDesc[5].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	// Create Input Layout
	D3DX11_PASS_DESC passDesc{};
	m_pEffect->GetTechnique()->GetPassByIndex(0)->GetDesc(&passDesc);

	HRESULT result = pDevice->CreateInputLayout
	(
		vertexDesc,
		numElements,
		passDesc.pIAInputSignature,
		passDesc.IAInputSignatureSize,
		&m_pInputLayout
	);

	if (FAILED(result))
	{
		std::cout << "failed to create inputlayout" << std::endl;
		return;
	}
		

	// Create vertex buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(Vertex) * static_cast<uint32_t>(vertices.size());
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices.data();

	result = pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
	if (FAILED(result))
	{
		std::cout << "failed to create Vertex Buffer" << std::endl;
		return;
	}

	// Create index buffer
	m_NumIndices = static_cast<uint32_t>(indices.size());
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(uint32_t) * m_NumIndices;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	initData.pSysMem = indices.data();
	result = pDevice->CreateBuffer(&bd,&initData,&m_pIndexBuffer);
	if (FAILED(result))
	{
		std::cout << "failed to index Vertex Buffer" << std::endl;
		return;
	}

	m_pEffect->SetMaterial(materialComponents, pDevice);
	
}

Mesh::~Mesh()
{
	if (m_pInputLayout)
	{
		m_pInputLayout->Release();
		m_pInputLayout = nullptr;
	}

	if (m_pVertexBuffer)
	{
		m_pVertexBuffer->Release();
		m_pVertexBuffer = nullptr;
	}
	
	if (m_pIndexBuffer)
	{
		m_pIndexBuffer->Release();
		m_pIndexBuffer = nullptr;
	}

	if (m_pEffect)
	{
		delete m_pEffect;
		m_pEffect = nullptr;
	}
}

void Mesh::Render(ID3D11DeviceContext* pDeviceContext, dae::Matrix& worldMatrix ,dae::Matrix& worldProjViewMatrix,const float* cameraPos) const
{
	// 1. Set Primitive Topology
	pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 2. Set Input Layout
	pDeviceContext->IASetInputLayout(m_pInputLayout);

	// 3. Set VertexBuffer
	constexpr UINT stride = sizeof(Vertex);
	constexpr UINT offset = 0;
	pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// 4. Set IndexBuffer
	pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// 4B. Set Matrixes and Camera
	m_pEffect->UpdateData(worldProjViewMatrix.GetData(),
										worldMatrix.GetData(),
										cameraPos);

	//5.Draw
	D3DX11_TECHNIQUE_DESC techDesc{};
	m_pEffect->GetTechnique()->GetDesc(&techDesc);

	for (UINT p = 0; p < techDesc.Passes; p++)
	{
		m_pEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, pDeviceContext);
		pDeviceContext->DrawIndexed(m_NumIndices, 0, 0);
	}

}

void Mesh::ToggleTechnique() const
{
	m_pEffect->ToggleTechnique();
}

PrimitiveTopology Mesh::GetPrimitiveTopology()
{
	return m_PrimitiveTopology;
}

void Mesh::SetPrimitiveTopology(const PrimitiveTopology primitiveTopologyType)
{
	m_PrimitiveTopology = primitiveTopologyType;
}

void Mesh::UpdateWorldMatrixRotY(float yaw, float deltaSeconds)
{
	float newyaw = yaw * deltaSeconds;
	m_WorldMatrix = m_WorldMatrix.CreateRotationY(newyaw);
}

dae::Matrix Mesh::GetWorldMatrix()
{
	return m_WorldMatrix;
}

void Mesh::SetWorldMatrix(dae::Matrix newMatrix)
{
	m_WorldMatrix = newMatrix;
}

MatCompFormat& Mesh::GetMaterialComponentByName(std::string directXVarName) const
{
	return m_pEffect->GetMaterial().GetMaterialComponentByName(directXVarName);
}

bool Mesh::HasMaterialByComponentName(std::string directXVarName) const
{
	return m_pEffect->GetMaterial().DoesMaterialComponentExistByName(directXVarName);
}

std::vector<VertexOut>& Mesh::GetOutVertices()
{
	return m_VerticesOut;
}

std::vector<Vertex>& Mesh::GetVertices()
{
	return m_Vertices;
}

std::vector<uint32_t>& Mesh::GetIndices()
{
	return m_Indices;
}
