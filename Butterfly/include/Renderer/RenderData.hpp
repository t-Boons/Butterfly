#pragma once
#include "Core/Common.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/Light.hpp"
#include "Renderer/ShadowMap.hpp"

#include "Core/Application.hpp"
#include "Scene/Registry/MeshRendererComponent.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Asset/AssetTypes.hpp"


#define MAX_LIGHTS 16

namespace Butterfly
{
	struct CameraData
	{
		glm::mat4 ViewProjection;
		glm::mat4 InverseProjection;
		glm::mat4 InverseView;
		glm::vec3 CameraPosition;
		float Near;
		float Far;
	};

	struct RenderObject
	{
		AssetHandle<MeshAsset> MeshAsset;
		MeshAsset::SubMesh SubMesh;
		entt::entity Entity;
		int ModelDataArrayIndex;
	};

	struct GPUModel
	{
		glm::mat4 ModelMatrix;
		glm::mat4 InverseModelMatrix;
		glm::mat4 NormalMatrix;

		glm::vec4 SDFBoundsMin;
		glm::vec4 SDFBoundsSize;
		glm::vec4 SDFResolution;

		int SDFTextureIndex;
	};

	class BFRenderData
	{
	public:
		CameraData UniformCameraData;
		RefPtr<BFUniformBuffer> Uniforms;
		MaterialLibrary Materials;
		LightBuffer Lights;
		RefPtr<BFStructuredBuffer> ModelDataBuffer;
		std::vector<GPUModel> ModelDataList;
		ShadowMapBuffer ShadowMaps;
		std::vector<RenderObject> CulledRenderObjects;
		uint32_t MeshesInScene = 0;

		std::array<std::array<BFFrustum, CUBEMAP_SLICES>, MAX_LIGHTS> ShadowLightFrustums;
		std::array<std::array<std::vector<RenderObject>, CUBEMAP_SLICES>, MAX_LIGHTS> ShadowRenderObjects;

	public:
		BFRenderData()
		{
			Uniforms = MakeRef<BFUniformBuffer>(4096, "Uniforms");

			{
				BFStructuredBufferDesc desc;
				desc.Data = nullptr;
				desc.HeapType = BFHeapType::Upload;
				desc.NumElements = 128;
				desc.Stride = sizeof(GPUModel);
				desc.DebugName = "ModelData";

				ModelDataBuffer = MakeRef<BFStructuredBuffer>(desc);
			}
		}

		void Update(const Viewport& viewport, const CameraData& cameraData)
		{
			BF_PROFILE_EVENT();

			UniformCameraData = cameraData;
			Uniforms->GetOrCreateView(sizeof(CameraData), HASH("CameraData"));
			Uniforms->Write(&cameraData, sizeof(CameraData), HASH("CameraData"));
			Lights.Update(viewport);
			ShadowMaps.Update(viewport);
			Materials.Validate();


			const uint32_t numLights = Lights.GetNumLights();
			const auto& shadowMaps = ShadowMaps.GetShadowMaps();

			BFFrustum cameraFrustum = BFFrustum{ cameraData.ViewProjection, {} };
			cameraFrustum.UpdatePlanes();

			CulledRenderObjects.clear();
			MeshesInScene = 0;

			// Clear shadow render lists once.
			for (int i = 0; i < numLights; ++i)
			{
				for (int j = 0; j < shadowMaps[i].NumSlices; ++j)
				{
					ShadowRenderObjects[i][j].clear();
					ShadowLightFrustums[i][j].Projection = shadowMaps[i].LightViewProjections[j];
					ShadowLightFrustums[i][j].UpdatePlanes();
				}
			}


			ModelDataList.clear();
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

				if (mesh->GetSDF())
				{
					model.SDFResolution = glm::vec4(mesh->GetSDF()->GetSDFResolution(), 0.0f);
					model.SDFTextureIndex = mesh->GetSDF()->GetSDFTexture()->SRV().View();
					model.SDFBoundsMin = glm::vec4(mesh->GetSDF()->GetSDFBounds().Min, 0.0f);
					model.SDFBoundsSize = glm::vec4(mesh->GetSDF()->GetSDFBounds().GetSize(), 0.0f);
				}
				else
				{
					model.SDFTextureIndex = -1;
				}

				ModelDataList.push_back(model);



				for (const auto& subm : mesh->GetSubMeshes())
				{
					MeshesInScene++;

					BFAABB worldAABB = subm.SubBounds.Transform(transform.GetWorldMatrix());

					// Camera culling
					if (worldAABB.IntersectsFrustum(cameraFrustum))
					{
						CulledRenderObjects.push_back({ meshRenderer.GetMeshHandle(), subm, entity, static_cast<int>(ModelDataList.size() - 1) });
					}

					// Shadow culling
					for (uint32_t i = 0; i < numLights; ++i)
					{
						for (int j = 0; j < shadowMaps[i].NumSlices; ++j)
						{
							if (worldAABB.IntersectsFrustum(ShadowLightFrustums[i][j]))
							{
								ShadowRenderObjects[i][j].push_back({ meshRenderer.GetMeshHandle(), subm, entity, static_cast<int>(ModelDataList.size() - 1) });
							}
						}
					}
				}
			}
			ModelDataBuffer->Write(ModelDataList.data(), sizeof(GPUModel) * static_cast<uint32_t>(ModelDataList.size()), 0);
		}
	};
}