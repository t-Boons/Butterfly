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

		TextureDesc desc;
		desc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
		desc.Width = viewport.Size().x;
		desc.Height = viewport.Size().y;
		desc.ViewTypes = TextureDesc::ViewType::ShaderResource | TextureDesc::ViewType::RenderTargetable;
		desc.DebugName = "ColorCorrected_CompositeRenderTarget";

		Texture* compCopy = builder.CreateTransientTexture("CompositeRenderTarget", desc);

		CCPassData* data = builder.AllocParameters<CCPassData>();
		builder.AddPass<CCPassData>("ColorspaceCorrectionPass", [&, compCopy](const CCPassData& data, D3D12CommandList& list)
			{
				RasterPassStartInfo info;
				info.RenderTargetLoadOp = LoadOP::Load;
				info.RenderTarget = compCopy;
				list.StartRenderPass(info, "ColorspaceCorrectionPass");

				list.SetViewport(RenderViewport::FromTexture(*compCopy));

				GraphicsPSOInfo pso;
				pso.Rasterizer.CullMode = D3D12_CULL_MODE_NONE;
				pso.VertexShader = ShaderCache::GetOrCreate("Fullscreen_vert.hlsl");
				pso.PixelShader = ShaderCache::GetOrCreate("CopyToSRGB_frag.hlsl");
				pso.DepthStencil.EnableDepth = false;

				list.SetGraphicsPSO(pso);

				viewport.GetRenderTarget().Resource()->TransitionAll(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				ShaderVariableBuilder(list)
					.Add(viewport.GetRenderTarget().SRV(SRVTextureViewInfo::Texture2D()))
					.Add(static_cast<int>(m_colorSpace))
					.Submit();

				list.DrawInstanced(6, 1, 0, 0);

				list.EndRenderPass();

				GraphicsCommands::Blit(list, *compCopy, viewport.GetRenderTarget());
			});
	}
}