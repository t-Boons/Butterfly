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
	namespace GPUStructs
	{
		struct FrameData
		{
			int MeshBuffer;
			int NumMeshes;

			int ModelBuffer;
			int NumModels;

			int LightBuffer;
			int NumLights;

			int ShadowMapBuffer;
			int NumShadowMaps;

			int MaterialBuffer;
			int NumMaterials;
			
			int CameraConstantData;
		};

		struct Mesh
		{
			int PositionBuffer = -1;
			int NormalBuffer = -1;
			int TangentBuffer = -1;
			int UVBuffer = -1;
		};

		struct Model
		{
			glm::mat4 ModelMatrix;
			glm::mat4 InverseModelMatrix;
			glm::mat4 NormalMatrix;

			glm::vec4 SDFBoundsMin;
			glm::vec4 SDFBoundsSize;
			glm::vec4 SDFResolution;

			int SDFTextureIndex;
		};

		struct CameraData
		{
			glm::mat4 ViewProjection;
			glm::mat4 InverseProjection;
			glm::mat4 InverseView;
			glm::vec3 CameraPosition;
			float Near;
			float Far;
		};
	}

	struct BFCameraData
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

	class BFRenderData
	{
	public:
		RefPtr<BFTypedConstantBuffer<GPUStructs::FrameData>> FrameDataBuffer;

		RefPtr<BFTypedConstantBuffer<GPUStructs::CameraData>> CameraDataBuffer;
		BFCameraData CameraData;

		MaterialLibrary Materials;
		LightBuffer Lights;
		RefPtr<BFTypedStructuredBuffer<GPUStructs::Model>> ModelDataBuffer;
		ShadowMapBuffer ShadowMaps;
		std::vector<RenderObject> CulledRenderObjects;
		RefPtr<BFTypedStructuredBuffer<GPUStructs::Mesh>> MeshDataBuffer;
		uint32_t MeshesInScene = 0;

		std::array<std::array<BFFrustum, CUBEMAP_SLICES>, MAX_LIGHTS> ShadowLightFrustums;
		std::array<std::array<std::vector<RenderObject>, CUBEMAP_SLICES>, MAX_LIGHTS> ShadowRenderObjects;

	public:
		BFRenderData()
		{
			{
				BFTypedStructuredBufferDesc desc;
				desc.Data = nullptr;
				desc.HeapType = BFHeapType::Upload;
				desc.NumElements = 128;

				ModelDataBuffer = MakeRef<BFTypedStructuredBuffer<GPUStructs::Model>>(desc);
			}

			{
				BFTypedStructuredBufferDesc desc;
				desc.Data = nullptr;
				desc.HeapType = BFHeapType::Upload;
				desc.NumElements = 128;

				MeshDataBuffer = MakeRef<BFTypedStructuredBuffer<GPUStructs::Mesh>>(desc);
			}

			{
				CameraDataBuffer = MakeRef<BFTypedConstantBuffer<GPUStructs::CameraData>>();
				FrameDataBuffer = MakeRef<BFTypedConstantBuffer<GPUStructs::FrameData>>();
			}
		}

		void Update(const Viewport& viewport, const BFCameraData& cameraData)
		{
			BF_PROFILE_EVENT();

			CameraData = cameraData;

			CameraDataBuffer->Data.ViewProjection = cameraData.ViewProjection;
			CameraDataBuffer->Data.InverseProjection = cameraData.InverseProjection;
			CameraDataBuffer->Data.InverseView = cameraData.InverseView;
			CameraDataBuffer->Data.CameraPosition = cameraData.CameraPosition;
			CameraDataBuffer->Data.Near = cameraData.Near;
			CameraDataBuffer->Data.Far = cameraData.Far;
			CameraDataBuffer->Write();

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

			MeshDataBuffer->Data.clear();
			ModelDataBuffer->Data.clear();
			auto view = Application::Get().GetScene().GetEntityRegistry().view<TransformComponent, MeshRendererComponent>();
			for (auto [entity, transform, meshRenderer] : view.each())
			{
				if (!meshRenderer.GetMeshHandle())
				{
					continue;
				}

				GPUStructs::Model model;
				GPUStructs::Mesh meshData;
				AssetManager& as = Application::Get().GetAssetManager();
				MeshAsset* mesh = as.Resolve<MeshAsset>(meshRenderer.GetMeshHandle());

				meshData.PositionBuffer = mesh->GPUPositions->SRV().View();
				meshData.NormalBuffer = mesh->GPUNormals ?  mesh->GPUNormals->SRV().View() : -1;
				meshData.TangentBuffer = mesh->GPUTangents ? mesh->GPUTangents->SRV().View() : -1;
				meshData.UVBuffer = mesh->GPUUVs ? mesh->GPUUVs->SRV().View() : -1;
				
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

				ModelDataBuffer->Data.push_back(model);
				MeshDataBuffer->Data.push_back(meshData);

				for (const auto& subm : mesh->GetSubMeshes())
				{
					MeshesInScene++;

					BFAABB worldAABB = subm.SubBounds.Transform(transform.GetWorldMatrix());

					// Camera culling
					if (worldAABB.IntersectsFrustum(cameraFrustum))
					{
						CulledRenderObjects.push_back({ meshRenderer.GetMeshHandle(), subm, entity, static_cast<int>(ModelDataBuffer->Data.size() - 1) });
					}

					// Shadow culling
					for (uint32_t i = 0; i < numLights; ++i)
					{
						for (int j = 0; j < shadowMaps[i].NumSlices; ++j)
						{
							if (worldAABB.IntersectsFrustum(ShadowLightFrustums[i][j]))
							{
								ShadowRenderObjects[i][j].push_back({ meshRenderer.GetMeshHandle(), subm, entity, static_cast<int>(ModelDataBuffer->Data.size() - 1) });
							}
						}
					}
				}
			}

			ModelDataBuffer->Write();
			MeshDataBuffer->Write();
			

			FrameDataBuffer->Data.MeshBuffer = MeshDataBuffer->SRV().View();
			FrameDataBuffer->Data.NumMeshes = static_cast<int>(MeshDataBuffer->Data.size());

			FrameDataBuffer->Data.ModelBuffer = ModelDataBuffer->SRV().View();
			FrameDataBuffer->Data.NumModels = static_cast<int>(ModelDataBuffer->Data.size());

			FrameDataBuffer->Data.LightBuffer = Lights.SRV().View();
			FrameDataBuffer->Data.NumLights = static_cast<int>(Lights.GetNumLights());

			FrameDataBuffer->Data.ShadowMapBuffer = ShadowMaps.SRV().View();
			FrameDataBuffer->Data.NumShadowMaps = static_cast<int>(ShadowMaps.GetNumShadowMaps());

			FrameDataBuffer->Data.MaterialBuffer = Materials.GetMaterialBuffer().SRV().View();
			FrameDataBuffer->Data.NumMaterials = static_cast<int>(Materials.GetNumMaterials());

			FrameDataBuffer->Data.CameraConstantData = CameraDataBuffer->CBV().View();

			FrameDataBuffer->Write();
		}
	};
}