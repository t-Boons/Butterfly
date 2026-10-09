#include "Renderer/RenderPipeline/ColorspaceCorrectionRenderPipelineStage.hpp"

namespace Butterfly
{
	void ColorspaceCorrectionRenderPipelineStage::OnRecordPass(const ViewportRenderEvent& ev)
	{
		struct CCPassData
		{
		};

		BF_PROFILE_EVENT()
			GraphBuilder& builder = ev.Builder;
		Viewport& viewport = ev.Viewport;

		BFTextureDesc desc;
		desc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
		desc.Width = viewport.Size().x;
		desc.Height = viewport.Size().y;
		desc.ViewTypes = BFTextureDesc::ViewType::ShaderResource | BFTextureDesc::ViewType::RenderTargettable;
		desc.DebugName = "ColorCorrected_CompositeRenderTarget";

		BFRGTexture* compCopy = builder.CreateTransientTexture("CompositeRenderTarget", desc);

		CCPassData* data = builder.AllocParameters<CCPassData>();
		builder.AddPass<CCPassData>("ColorspaceCorrectionPass", [&, compCopy](const CCPassData& data, D3D12CommandList& list)
			{
				RasterPassStartInfo info;
				info.RenderTargetLoadOp = LoadOP::Load;
				info.RenderTarget = compCopy->Resource().get();
				list.StartRenderPass(info, "ColorspaceCorrectionPass");

				list.SetViewport(RenderViewport::FromTexture(*compCopy->Resource()));

				BFGraphicsPSOInfo pso;
				pso.Rasterizer.CullMode = D3D12_CULL_MODE_NONE;
				pso.VertexShader = BFShaderCache::GetOrCreate(L"assets/Shaders/Fullscreen_vert.hlsl", ShaderType::Vertex);
				pso.PixelShader = BFShaderCache::GetOrCreate(L"assets/Shaders/CopyToSRGB_frag.hlsl", ShaderType::Pixel);
				pso.DepthStencil.EnableDepth = false;

				list.SetGraphicsPSO(pso);

				viewport.GetRenderTarget().Resource()->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				ShaderVariables()
					.Add(viewport.GetRenderTarget().SRV().View())
					.Add(static_cast<int>(m_colorSpace))
					.Submit(list);

				list.DrawInstanced(6, 1, 0, 0);

				list.EndRenderPass();

				GraphicsCommands::Blit(list, *compCopy->Resource(), viewport.GetRenderTarget());
			});
	}
}