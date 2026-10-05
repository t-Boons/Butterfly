#include "Renderer/RenderPipeline/ClearRenderPipelineStage.hpp"

namespace Butterfly
{
	void ClearRenderPipelineStage::OnRecordPass(const ViewportRenderEvent& ev)
	{
		struct ClearParams
		{
		};
		const auto params = ev.Builder.AllocParameters<ClearParams>();

		ev.Builder.AddPass<ClearParams>("Clear Composite Render Target",
			[&](const ClearParams& params, D3D12CommandList& list)
			{
				RasterPassStartInfo info;
				info.RenderTarget = &ev.Viewport.GetRenderTarget();
				info.DepthStencil = &ev.Viewport.GetDepthStencil();
				info.ClearColor = { 0.1f, 0.02f, 0.02f, 1.0f };
				info.DepthValue = 1.0f;
				info.DepthStencilLoadOp = LoadOP::Clear;
				info.RenderTargetLoadOp = LoadOP::Clear;
				list.StartRenderPass(info, "Clear Composite Render Target Pass");
				list.EndRenderPass();
			});
	}
}