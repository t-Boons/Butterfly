#pragma once
#include "Core/Common.hpp"
#include "Renderer/D3D12/D3D12Common.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/D3d12Buffer.hpp"
#include "Renderer/Graph/Graph.hpp"
#include "Renderer/D3D12/D3D12Fence.hpp"
#include "Renderer/Graph/Blackboard.hpp"
#include "Core/Window.hpp"
#include "Renderer/ModelLoading/ModelImporter.hpp"
#include "Core/Application.hpp"

#include "Renderer/GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12DescriptorAllocator.hpp"
#include "Renderer/D3D12/D3D12CommandQueue.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12GraphicsCommands.hpp"
#include "Renderer/D3D12/D3D12Shader.hpp"
#include "Renderer/D3D12/D3D12ShaderVariables.hpp"
#include "Renderer/D3D12Sampler.hpp"
#include "Renderer/D3D12/D3D12Pipeline.hpp"
#include "Renderer/Graph/GraphBuilder.hpp"
#include "Renderer/D3D12/D3D12View.hpp"

namespace Butterfly
{
	class MeshRenderer : public NonCopyable
	{
	public:
		void LoadTestModel()
		{
			// Load the test model.
			RefPtr<ModelImporter> importer = ModelImporter::Create("assets/Models/damagedhelmet/DamagedHelmet.gltf");
			importer->Load();

			auto& material = importer->Materials()[0];

			// Model texture(s)
			BFTextureDesc desc;
			desc.Flags = BFTextureDesc::ShaderResource;
			desc.Width = material->m_colorTexture->m_width;
			desc.Height = material->m_colorTexture->m_height;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;


			m_modelAlbedo = BFTexture::CreateTextureFromCPUBuffer(
				desc,
				material->m_colorTexture->m_image.data(),
				material->m_name);

			// Model Indices
			auto& mesh = importer->Meshes()[0];

			m_modelIndices = ScopePtr<BFIndexBuffer>(new BFIndexBuffer(mesh->m_indices[0].data(), static_cast<uint32_t>(mesh->m_indices[0].size()), DXGI_FORMAT_R32_UINT, "ModelIndices"));

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

			{
				srvDesc.Buffer.StructureByteStride = sizeof(glm::vec3);
				srvDesc.Buffer.NumElements = static_cast<uint32_t>(mesh->m_positions[0].size());
				const auto size = static_cast<uint32_t>(mesh->m_positions[0].size() * sizeof(glm::vec3));
				m_modelPositions = ScopePtr<BFStructuredBuffer>(new BFStructuredBuffer(mesh->m_positions[0].data(), size, &srvDesc, "Position"));
			}

			{
				srvDesc.Buffer.StructureByteStride = sizeof(glm::vec3);
				srvDesc.Buffer.NumElements = static_cast<uint32_t>(mesh->m_normals[0].size());
				const auto size = static_cast<uint32_t>(mesh->m_normals[0].size() * sizeof(glm::vec3));
				m_modelNormals = ScopePtr<BFStructuredBuffer>(new BFStructuredBuffer(mesh->m_normals[0].data(), size, &srvDesc, "Normals"));
			}
			{
				srvDesc.Buffer.StructureByteStride = sizeof(glm::vec2);
				srvDesc.Buffer.NumElements = static_cast<uint32_t>(mesh->m_texcoords[0].size());
				const auto size = static_cast<uint32_t>(mesh->m_texcoords[0].size() * sizeof(glm::vec2));
				m_modelUVS = ScopePtr<BFStructuredBuffer>(new BFStructuredBuffer(mesh->m_texcoords[0].data(), size, &srvDesc, "TexCoords"));
			}
		}

		ScopePtr<BFIndexBuffer> m_modelIndices;
		ScopePtr<BFStructuredBuffer> m_modelPositions;
		ScopePtr<BFStructuredBuffer> m_modelNormals;
		ScopePtr<BFStructuredBuffer> m_modelUVS;
		RefPtr<BFTexture> m_modelAlbedo;
	};
}
