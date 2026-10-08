#pragma once
#include "Core/Common.hpp"
#include "Core/Application.hpp"
#include "Scene/Registry/MeshRendererComponent.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Renderer/D3D12Buffer.hpp"

namespace Butterfly
{
	struct GPUModel
	{
		glm::mat4 ModelMatrix;
		glm::mat4 InverseModelMatrix;
		glm::mat4 NormalMatrix;

		glm::vec4 BoundsMin;
		glm::vec4 BoundsSize;
		glm::vec4 SDFResolution;
		int SDFTextureIndex;
	};


	class ModelBuffer
	{
	public:
		ModelBuffer()
		{
			BFStructuredBufferDesc desc;
			desc.Data = nullptr;
			desc.HeapType = BFHeapType::Upload;
			desc.NumElements = 128;
			desc.Stride = sizeof(GPUModel);
			desc.DebugName = "ModelData";

			m_modelBuffer = MakeRef<BFStructuredBuffer>(desc);
		}

		void Update()
		{
			m_models.clear();

			uint32_t entityIndex = 0;
			auto view = Application::Get().GetScene().GetEntityRegistry().view<TransformComponent, MeshRendererComponent>();
			for (auto [entity, transform, meshRenderer] : view.each())
			{
				if (!meshRenderer.GetMeshHandle())
				{
					continue;
				}

				GPUModel model;

				AssetManager& as = Application::Get().GetAssetManager();
				MeshAsset* mesh = as.Resolve<MeshAsset>(meshRenderer.GetMeshHandle());

				model.ModelMatrix = transform.GetWorldMatrix();
				model.InverseModelMatrix = glm::inverse(model.ModelMatrix);
				model.NormalMatrix = glm::transpose(glm::inverse(model.ModelMatrix));
				model.BoundsMin = glm::vec4(mesh->Bounds.Min, 0.0f);
				model.BoundsSize = glm::vec4(mesh->Bounds.Size(), 0.0f);

				if (mesh->SDF)
				{
					model.SDFResolution = glm::vec4(mesh->SDFResolution, 0.0f);
					model.SDFTextureIndex = mesh->SDF->SRV().View();
				}
				else
				{
					model.SDFResolution = glm::vec4(0.0f);
					model.SDFTextureIndex = -1;
				}

				m_models.push_back(model);

				entityIndex++;
			}

			m_modelBuffer->Write(m_models.data(), sizeof(GPUModel) * GetNumModels(), 0);
		}

		uint32_t GetModelViewIndex(uint32_t entityIndex)
		{
			return entityIndex;
		}

		uint32_t GetNumModels() const
		{
			return static_cast<uint32_t>(m_models.size());
		}

		const BFShaderResourceView& SRV() const
		{
			return m_modelBuffer->SRV();
		}

	private:
		RefPtr<BFStructuredBuffer> m_modelBuffer;
		std::vector<GPUModel> m_models;
	};
}