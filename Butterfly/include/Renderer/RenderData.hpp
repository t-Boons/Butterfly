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

		std::array<std::array<std::vector<RenderObject>, MAX_LIGHTS>, CUBEMAP_SLICES> ShadowRenderObjects;

	public:
		BFRenderData()
		{
			Uniforms = MakeRef<BFUniformBuffer>(4096, "Uniforms");
		}

		void Update(const Viewport& viewport, const CameraData& cameraData)
		{
			UniformCameraData = cameraData;
			Uniforms->GetOrCreateView(sizeof(CameraData), HASH("CameraData"));
			Uniforms->Write(&cameraData, sizeof(CameraData), HASH("CameraData"));
			Lights.Update(viewport);
			ShadowMaps.Update(viewport);
			Materials.Validate();
			Models.Validate();

			CulledRenderObjects.clear();
			MeshesInScene = 0;
			for (const auto& [entity, meshRenderer, transform] : Application::Get().GetScene().GetEntityRegistry().view<MeshRendererComponent, TransformComponent>().each())
			{
				if (!meshRenderer.GetMeshHandle())
				{
					continue;
				}

				const MeshAsset* mesh = Application::Get().GetAssetManager().Resolve<MeshAsset>(meshRenderer.GetMeshHandle());
				const glm::mat4 modelMatrix = transform.GetWorldMatrix();
				for (const auto& subm : mesh->GetSubMeshes())
				{
					const std::array<glm::vec3, 8> corners = subm.SubBounds.GetCorners();
					BFAABB worldAABB{};
					for (const glm::vec3& corner : corners)
					{
						const glm::vec3 worldCorner = glm::vec3(modelMatrix * glm::vec4(corner, 1.0f));
						worldAABB.ExpandToFit(worldCorner);
					}

					if (worldAABB.IntersectsFrustum(cameraData.ViewProjection))
					{
						CulledRenderObjects.push_back({ meshRenderer.GetMeshHandle(), subm, entity });
					}
					MeshesInScene++;
				}

				for (int i = 0; i < MAX_LIGHTS; ++i)
				{
					for (int j = 0; j < CUBEMAP_SLICES; ++j)
					{
						ShadowRenderObjects[i][j].clear();
					}

					if (i >= Lights.GetNumLights())
					{
						break;
					}

					for (const auto& subm : mesh->GetSubMeshes())
					{
						const std::array<glm::vec3, 8> corners = subm.SubBounds.GetCorners();
						BFAABB worldAABB{};
						for (const glm::vec3& corner : corners)
						{
							const glm::vec3 worldCorner = glm::vec3(modelMatrix * glm::vec4(corner, 1.0f));
							worldAABB.ExpandToFit(worldCorner);
						}

						for (int j = 0; j < ShadowMaps.GetShadowMaps()[i].NumSlices; ++j)
						{
							if (worldAABB.IntersectsFrustum(ShadowMaps.GetShadowMaps()[i].LightViewProjections[j]))
							{
								ShadowRenderObjects[i][j].push_back({ meshRenderer.GetMeshHandle(), subm, entity });
							}
						}
					}
				}
			}
		}
	};
}