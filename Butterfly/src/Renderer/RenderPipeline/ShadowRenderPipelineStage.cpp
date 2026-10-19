#pragma once
#include "Renderer/RenderPipeline/ShadowRenderPipelineStage.hpp"
#include "Renderer/RenderPipeline/RenderPipeline.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/D3D12Sampler.hpp"
#include "Renderer/D3D12/D3D12CommandQueue.hpp"

namespace Butterfly
{
	void ShadowRenderPipelineStage::OnRecordPass(const ViewportRenderEvent& ev)
	{
		GraphBuilder& builder = ev.Builder;
		Viewport& viewport = ev.Viewport;


		BFTextureDesc desc;
		desc.Width = 512;
		desc.Height = 512;
		desc.Format = DXGI_FORMAT_R32_FLOAT;
		desc.ViewTypes = BFTextureDesc::ViewType:DepthStencilState | BFTextureDesc::ViewType::ShaderResource;
		desc.DebugName = "ShadowMap";
		desc.Type = BFTextureType::Texture2D;
		params->Shadowmap = builder.CreateTransientTexture("ShadowMap", desc);

		ShadowMapParameters* params = builder.AllocParameters<ShadowMapParameters>();




		ForwardRenderer* forwardParams = builder.GetPassData<ShadowMapParameters, ForwardRenderer>();

		builder.AddPass<ShadowMapParameters>("ShadowMap",
			[forwardParams, viewport, this](const ShadowMapParameters& params, D3D12CommandList& list)
			{
				BF_PROFILE_EVENT_DYNAMIC("ShadowMap pass");

				RasterPassStartInfo info;
				info.DepthStencil = &viewport.GetDepthStencil();
				info.DepthStencilLoadOp = LoadOP::Clear;
				list.StartRenderPass(info, "ShadowMap Pass");

				list.SetViewport(RenderViewport::FromTexture(params.Shadowmap));

				BFGraphicsPSOInfo pso;
				pso.Rasterizer.CullMode = D3D12_CULL_MODE_BACK;
				pso.DepthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
				pso.DepthStencil.WriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
				pso.DepthStencil.EnableDepth = true;
				pso.PixelShader = BFShaderCache::GetOrCreate(L"assets/Shaders/ShadowMap_frag.hlsl", ShaderType::Pixel);
				pso.VertexShader = BFShaderCache::GetOrCreate(L"assets/Shaders/ShadowMap_vert.hlsl", ShaderType::Vertex);
				list.SetGraphicsPSO(pso);

				BFSampler sampler;

				ShaderVariables()
					.Add(viewport.Uniforms->GetView(HASH("CameraData"))->View())
					.Add(sampler.View())
					.Add(m_skyboxTexture->SRV().View())
					.Submit(list);

				list.DrawInstanced(6, 1, 0, 0);

				list.EndRenderPass();
			});
	}
}