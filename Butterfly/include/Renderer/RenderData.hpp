#pragma once
#include "Core/Common.hpp"
#include "Renderer/Light.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/Model.hpp"
#include "Renderer/ShadowMap.hpp"

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

	class BFRenderData
	{
	public:
		CameraData UniformCameraData;
		RefPtr<BFUniformBuffer> Uniforms;
		MaterialLibrary Materials;
		LightBuffer Lights;
		ModelBuffer Models;
		ShadowMapBuffer ShadowMaps;

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
		}
	};
}