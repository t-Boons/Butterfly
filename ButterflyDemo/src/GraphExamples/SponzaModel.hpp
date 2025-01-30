#pragma once
#include "Butterfly.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/color_space.hpp"

namespace GraphExamples
{
	using namespace Butterfly;

	struct PointLight
	{
		glm::vec3 position;
		glm::vec3 color;
		float radius;
		float intensity;
	};


	struct Sphere
	{
		static Sphere* Get()
		{
			if (!s_model)
				s_model = new Sphere();

			return s_model;
		}

		Sphere()
		{
			RefPtr<ModelImporter> importer = ModelImporter::Create("assets/Models/Sphere/Sphere.gltf");
			importer->Load();

			const auto size = static_cast<uint32_t>(importer->Meshes()[0]->m_indices[0].size());
			m_indices = ScopePtr<BFIndexBuffer>(new BFIndexBuffer(importer->Meshes()[0]->m_indices[0].data(), size, DXGI_FORMAT_R32_UINT, "Sphere Indices"));


			const auto size1 = static_cast<uint32_t>(importer->Meshes()[0]->m_positions[0].size());

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = size1;
			srvDesc.Buffer.StructureByteStride = sizeof(glm::vec3);
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

			m_positions = ScopePtr<BFStructuredBuffer>(new BFStructuredBuffer(importer->Meshes()[0]->m_positions[0].data(), size1 * sizeof(glm::vec3), &srvDesc, "Sphere Positions"));
		}

		inline static Sphere* s_model = nullptr;

		ScopePtr<BFStructuredBuffer> m_positions;
		ScopePtr<BFIndexBuffer> m_indices;
	};


	struct SponzaModel
	{
		static SponzaModel* Get()
		{
			if (!s_model) 
				s_model = new SponzaModel();

			return s_model;
		}

		inline static SponzaModel* s_model = nullptr;


		SponzaModel()
		{
			RefPtr<ModelImporter> importer = ModelImporter::Create("assets/Models/Sponza/Sponza.gltf");
			importer->Load();



			const uint32_t numMeshes = static_cast<uint32_t>(importer->Meshes().size());
			ModelMatrix = importer->Root()->m_children[0]->m_matrix;


			auto& materials = importer->Materials();

			for (auto& material : materials)
			{
				BFTextureDesc desc;
				desc.Flags = BFTextureDesc::ShaderResource;
				desc.Width = material->m_colorTexture->m_width;
				desc.Height = material->m_colorTexture->m_height;
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

				BFTexture* ptr = BFTexture::CreateTextureFromCPUBuffer(
					desc,
					material->m_colorTexture->m_image.data(),
					material->m_name
				);

				Textures.push_back(ScopePtr<BFTexture>(ptr));
			}

			for (size_t i = 0; i < numMeshes; i++)
			{
				auto& model = importer->Models()[i];
				auto& mesh = importer->Meshes()[i];


				Meshes.push_back(static_cast<uint32_t>(mesh->m_indices.size() - 1));
				IndexBuffers.push_back(PrimitiveIb());
				Positions.push_back(Primitive());
				Normals.push_back(Primitive());
				TexCoords.push_back(Primitive());

				for (auto& index : model->m_materialIndices)
				{
					TextureIndices.push_back(index);
				}

				for (auto& ib : mesh->m_indices)
				{
					const auto size = static_cast<uint32_t>(ib.size());
					IndexBuffers[i].push_back(ScopePtr<BFIndexBuffer>(new BFIndexBuffer(ib.data(), size, DXGI_FORMAT_R32_UINT, "IndexBuffer" + std::to_string(i))));
				}

				for (auto& pos : mesh->m_positions)
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					srvDesc.Format = DXGI_FORMAT_UNKNOWN;
					srvDesc.Buffer.FirstElement = 0;
					srvDesc.Buffer.NumElements = static_cast<uint32_t>(pos.size());
					srvDesc.Buffer.StructureByteStride = sizeof(glm::vec3);
					srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

					const auto size = static_cast<uint32_t>(pos.size() * sizeof(glm::vec3));
					Positions[i].push_back(ScopePtr<BFStructuredBuffer>(new BFStructuredBuffer(pos.data(), size, &srvDesc, "Position" + std::to_string(i))));
				}

				for (auto& norm : mesh->m_normals)
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					srvDesc.Format = DXGI_FORMAT_UNKNOWN;
					srvDesc.Buffer.FirstElement = 0;
					srvDesc.Buffer.NumElements = static_cast<uint32_t>(norm.size());
					srvDesc.Buffer.StructureByteStride = sizeof(glm::vec3);
					srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

					const auto size = static_cast<uint32_t>(norm.size() * sizeof(glm::vec3));
					Normals[i].push_back(ScopePtr<BFStructuredBuffer>(new BFStructuredBuffer(norm.data(), size, &srvDesc, "Normals" + std::to_string(i))));
				}

				for (auto& texc : mesh->m_texcoords)
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					srvDesc.Format = DXGI_FORMAT_UNKNOWN;
					srvDesc.Buffer.FirstElement = 0;
					srvDesc.Buffer.NumElements = static_cast<uint32_t>(texc.size());
					srvDesc.Buffer.StructureByteStride = sizeof(glm::vec2);
					srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

					const auto size = static_cast<uint32_t>(texc.size() * sizeof(glm::vec2));
					TexCoords[i].push_back(ScopePtr<BFStructuredBuffer>(new BFStructuredBuffer(texc.data(), size, &srvDesc, "TexCoords" + std::to_string(i))));
				}
			}


			std::vector<PointLight> lights;
			srand(time(0));
			for (int x = -16; x < 16; x++) for (int y = 0; y < 8; y++) for (float z = -1.5; z < 1.5f; z += 0.75f) // 1024 lights
			{
				PointLight p;
				p.color = glm::normalize(glm::rgbColor(glm::vec3{ ((float)x + 16.0f) / 32.0f * 225.0f, 1.0f, 1.0f }));
				p.position = glm::vec3{ (float)x * 0.5f, (float)y + 0.1f, (float)z };
				p.intensity = 0.05f;
				p.radius = std::sqrt(p.intensity / 0.01f) * 1.2f;
				lights.push_back(p);
			}
			numLights = lights.size();
			
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = static_cast<uint32_t>(lights.size());
			srvDesc.Buffer.StructureByteStride = sizeof(PointLight);
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

			m_lights = ScopePtr<BFStructuredBuffer>(new BFStructuredBuffer(lights.data(), (uint32_t)(lights.size() * sizeof(PointLight)), &srvDesc, "Lights"));
		}

		using PrimitiveIb = std::vector<ScopePtr<BFIndexBuffer>>;
		using Primitive = std::vector<ScopePtr<BFStructuredBuffer>>;

		std::vector<PrimitiveIb> IndexBuffers;
		std::vector<Primitive> Positions;
		std::vector<Primitive> Normals;
		std::vector<Primitive> TexCoords;

		std::vector<ScopePtr<BFTexture>> Textures;
		std::vector<uint32_t> TextureIndices;

		std::vector<uint32_t> Meshes;
		glm::mat4 ModelMatrix;
		inline static int numLights = 0;
		ScopePtr<BFStructuredBuffer> m_lights;
	};
}