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
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Width = viewport.Size().x;
		desc.Height = viewport.Size().y;
		desc.Flags = BFTextureDesc::ShaderResource | BFTextureDesc::RenderTargettable;
		desc.DebugName = "ColorCorrected_CompositeRenderTarget";

		BFRGTexture* compCopy = builder.CreateTransientTexture("CompositeRenderTarget", desc);

		CCPassData* data = builder.AllocParameters<CCPassData>();
		builder.AddPass<CCPassData>("ColorspaceCorrectionPass", [&, compCopy](const CCPassData& data, D3D12CommandList& list)
			{
				list.List()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				GraphicsCommands::SetRenderTargets(list, { compCopy->Resource().get() }, nullptr);

				GraphicsCommands::SetFullscreenViewportAndRect(list, viewport.Size().x, viewport.Size().y);

				BFPipelineBuilder psoBuilder;
				psoBuilder.PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
				psoBuilder.RenderTargetFormats({ DXGI_FORMAT_R8G8B8A8_UNORM });
				psoBuilder.VertexShader(BFShaderCache::GetOrCreate(L"assets/Shaders/Fullscreen_vert.hlsl", ShaderType::Vertex));
				psoBuilder.PixelShader(BFShaderCache::GetOrCreate(L"assets/Shaders/CopyToSRGB_frag.hlsl", ShaderType::Pixel));
				psoBuilder.DepthEnable(false);
				psoBuilder.CullingMode(D3D12_CULL_MODE_NONE);

				list.List()->SetPipelineState(psoBuilder.Create().GetHW());

				viewport.GetRenderTarget().Resource()->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				ShaderVariables()
					.Add(viewport.GetRenderTarget().SRV().View())
					.Add(static_cast<int>(m_colorSpace))
					.Submit(list);

				list.DrawInstanced(6, 1, 0, 0);

				GraphicsCommands::Blit(list, *compCopy->Resource(), viewport.GetRenderTarget());

			});
	}
}