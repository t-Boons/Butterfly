#pragma once
#include "Core/Common.hpp"
#include "Renderer/Light.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/Model.hpp"
#include "Renderer/ShadowMap.hpp"

#include "Core/Application.hpp"
#include "Scene/Registry/MeshRendererComponent.hpp"
#include "Asset/AssetTypes.hpp"

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
	};

	class BFRenderData
	{
	public:
		CameraData UniformCameraData;
		RefPtr<BFUniformBuffer> Uniforms;
		MaterialLibrary Materials;
		LightBuffer Lights;
		ModelBuffer Models;
		ShadowMapBuffer ShadowMaps;
		std::vector<RenderObject> CulledRenderObjects;
		uint32_t MeshesInScene = 0;

		std::array<std::array<BFFrustum, CUBEMAP_SLICES>, MAX_LIGHTS> ShadowLightFrustums;
		std::array<std::array<std::vector<RenderObject>, CUBEMAP_SLICES>, MAX_LIGHTS> ShadowRenderObjects;

	public:
		BFRenderData()
		{
			Uniforms = MakeRef<BFUniformBuffer>(4096, "Uniforms");
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
			Models.Validate();



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
#define USE_FRUSTUM_CULLING
#ifdef USE_FRUSTUM_CULLING
			for (const auto& [entity, meshRenderer, transform] :
				Application::Get().GetScene().GetEntityRegistry().view<MeshRendererComponent, TransformComponent>().each())
			{
				if (!meshRenderer.GetMeshHandle())
					continue;

				const MeshAsset* mesh = Application::Get().GetAssetManager().Resolve<MeshAsset>(meshRenderer.GetMeshHandle());
				const glm::mat4 modelMatrix = transform.GetWorldMatrix();

				for (const auto& subm : mesh->GetSubMeshes())
				{
					MeshesInScene++;

					BFAABB worldAABB = subm.SubBounds.Transform(modelMatrix);

					// Camera culling
					if (worldAABB.IntersectsFrustum(cameraFrustum))
					{
						CulledRenderObjects.push_back({ meshRenderer.GetMeshHandle(), subm, entity });
					}

					// Shadow culling
					for (uint32_t i = 0; i < numLights; ++i)
					{
						for (int j = 0; j < shadowMaps[i].NumSlices; ++j)
						{
							if (worldAABB.IntersectsFrustum(ShadowLightFrustums[i][j]))
							{
								ShadowRenderObjects[i][j].push_back({ meshRenderer.GetMeshHandle(), subm, entity });
							}
						}
					}
				}
			}
#else
			for (const auto& [entity, meshRenderer, transform] :
				Application::Get().GetScene().GetEntityRegistry().view<MeshRendererComponent, TransformComponent>().each())
			{
				if (!meshRenderer.GetMeshHandle())
					continue;

				const MeshAsset* mesh = Application::Get().GetAssetManager().Resolve<MeshAsset>(meshRenderer.GetMeshHandle());
				const glm::mat4 modelMatrix = transform.GetWorldMatrix();

				for (const auto& subm : mesh->GetSubMeshes())
				{
					MeshesInScene++;

					BFAABB worldAABB = subm.SubBounds.Transform(modelMatrix);
					CulledRenderObjects.push_back({ meshRenderer.GetMeshHandle(), subm, entity });

					// Shadow culling
					for (uint32_t i = 0; i < numLights; ++i)
					{
						for (int j = 0; j < shadowMaps[i].NumSlices; ++j)
						{
							ShadowRenderObjects[i][j].push_back({ meshRenderer.GetMeshHandle(), subm, entity });
						}
					}
				}
			}
#endif
		}
	};
}