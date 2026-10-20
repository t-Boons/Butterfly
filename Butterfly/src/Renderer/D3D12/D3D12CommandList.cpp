#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/D3D12/D3D12Resource.hpp"
#include "Renderer/D3D12/D3D12View.hpp"
#include "Renderer/D3D12/D3D12Pipeline.hpp"
#include "Renderer/D3D12/D3D12GraphicsCommands.hpp"
#include "Renderer/D3D12Buffer.hpp"


namespace Butterfly
{
	namespace Utils
	{
		inline D3D_PRIMITIVE_TOPOLOGY GetPrimitiveTopologyFromType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
		{
			switch (type)
			{
			case D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT:
				return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			case D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE:
				return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			case D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH:
				return D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
			default:
				BF_CORE_LOG_CRITICAL("Unknown primitive topology type: %d", static_cast<int>(type));
				return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
			}
		}
	}

	RenderViewport RenderViewport::FromTexture(const BFTexture& texture)
	{
		RenderViewport viewport;
		viewport.Width = texture.Desc().Width;
		viewport.Height = texture.Desc().Height;
		return viewport;
	}

	D3D12CommandList::D3D12CommandList(D3D12_COMMAND_LIST_TYPE type)
		: m_hasExecuted(false), m_cmdListClosed(false)
	{
		BF_PROFILE_EVENT();

		ThrowIfFailed(D3D12API()->Device()->CreateCommandAllocator(type, IID_PPV_ARGS(&m_allocator)));
		ThrowIfFailed(D3D12API()->Device()->CreateCommandList(0, type, m_allocator, nullptr, IID_PPV_ARGS(&m_cmdList)));
	}

	D3D12CommandList::~D3D12CommandList()
	{
		BF_PROFILE_EVENT();

		COM_FREE(m_cmdList);
		COM_FREE(m_allocator);
	}

	void D3D12CommandList::Reset()
	{
		BF_PROFILE_EVENT();

		if (!m_cmdListClosed)
		{
			m_cmdList->Close();
		}

		m_hasExecuted = false;
		m_allocator->Reset();
		m_cmdList->Reset(m_allocator, nullptr);

		m_cmdListClosed = false;


		m_bindlessBound = false;
		m_currentPassType = PassType::None;
		m_passRecording = false;
		m_passName = "";
	}

	void D3D12CommandList::Close()
	{
		BF_PROFILE_EVENT();

		if (!m_cmdListClosed)
		{
			m_cmdList->Close();
			m_cmdListClosed = true;
			return;
		}
	}

	void D3D12CommandList::Marker(const std::string& str)
	{
		m_cmdList->SetMarker(1u, str.c_str(), static_cast<uint32_t>(str.size() + 1));
	}

	void D3D12CommandList::BeginGPUMarker(const std::string& str)
	{
		m_cmdList->BeginEvent(1u, str.c_str(), static_cast<uint32_t>(str.size() + 1));
	}

	void D3D12CommandList::StartRenderPass(const RasterPassStartInfo& info, const std::string& name)
	{
		BF_PROFILE_EVENT();

		BF_CORE_ASSERT(!m_passRecording, "D3D12CommandList::StartRenderPass called while a pass is already recording. Call EndRenderPass on: %s", m_passName.c_str());
		m_passRecording = true;
		m_currentPassType = PassType::Raster;

		if (!m_bindlessBound)
		{
			GraphicsCommands::SetBindlessDescriptorHeapsAndRootSignature(*this, false);
			m_bindlessBound = true;
		}

		BeginGPUMarker(name);
		m_passName = name;

		if (info.RenderTarget)
		{
			SetColorTargets({ info.RenderTarget });

			if (info.RenderTargetLoadOp == LoadOP::Clear)
			{
				m_cmdList->ClearRenderTargetView(info.RenderTarget->RTV().Handle(), &info.ClearColor[0], 0, nullptr);
			}
		}

		if (info.DepthStencil)
		{
			SetDepthStencilTarget(*info.DepthStencil);

			if (info.DepthStencilLoadOp == LoadOP::Clear)
			{
				m_cmdList->ClearDepthStencilView(info.DepthStencil->DSV().Handle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, info.DepthValue, 0, 0, nullptr);
			}
		}
	}

	void D3D12CommandList::SetColorTarget(BFTexture& color)
	{
		SetColorTargets({ &color });
	}

	void D3D12CommandList::SetColorTargets(const std::vector<BFTexture*>& color)
	{
		m_colorTexture.clear();
		m_colorTextureView.clear();
		for (auto& c : color)
		{
			c->Resource()->Transition(*this, D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_colorTexture.push_back(c);
			m_colorTextureView.push_back(c->RTV().Handle());
		}

		D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle = m_depthStencilTexture ? &m_depthStencilView : nullptr;
		m_cmdList->OMSetRenderTargets(static_cast<uint32_t>(m_colorTextureView.size()), m_colorTextureView.data(), false, dsvHandle);
	}

	void D3D12CommandList::SetDepthStencilTarget(BFTexture& depthStencil, uint32_t slice)
	{
		m_depthStencilTexture = &depthStencil;
		m_depthStencilView = depthStencil.DSVArraySlice(slice).Handle();
		depthStencil.Resource()->Transition(*this, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle = m_depthStencilTexture ? &m_depthStencilView : nullptr;
		m_cmdList->OMSetRenderTargets(static_cast<uint32_t>(m_colorTextureView.size()), m_colorTextureView.data(), false, &m_depthStencilView);
	}

	void D3D12CommandList::ClearRenderTarget(BFTexture& color, const glm::vec4& clearColor)
	{
		color.Resource()->Transition(*this, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_cmdList->ClearRenderTargetView(color.RTV().Handle(), &clearColor[0], 0, nullptr);
	}

	void D3D12CommandList::ClearDepthStencilTarget(BFTexture& depthStencil, float depthValue, uint8_t stencilValue, uint32_t slice)
	{
		if(depthStencil.Format() == DXGI_FORMAT_D32_FLOAT)
		{
			m_cmdList->ClearDepthStencilView(depthStencil.DSVArraySlice(slice).Handle(), D3D12_CLEAR_FLAG_DEPTH, depthValue, stencilValue, 0, nullptr);
		}
		else
		{
			m_cmdList->ClearDepthStencilView(depthStencil.DSV().Handle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depthValue, stencilValue, 0, nullptr);
		}
	}

	void D3D12CommandList::StartComputePass(const std::string& name)
	{
		BF_PROFILE_EVENT();
		
		BF_CORE_ASSERT(!m_passRecording, "D3D12CommandList::StartComputePass called while a pass is already recording. Call EndCompute pass on: %s", m_passName.c_str());
		m_passRecording = true;
		m_currentPassType = PassType::Compute;
		
		BeginGPUMarker(name);
		m_passName = name;

		if (!m_bindlessBound)
		{
			GraphicsCommands::SetBindlessDescriptorHeapsAndRootSignature(*this, true);
			m_bindlessBound = true;
		}
	}

	void D3D12CommandList::EndComputePass()
	{
		BF_PROFILE_EVENT();
		BF_CORE_ASSERT(m_passRecording, "D3D12CommandList::EndComputePass called without a matching StartComputePass.");
		BF_CORE_ASSERT(m_currentPassType == PassType::Compute, "D3D12CommandList::EndComputePass called without a matching StartComputePass.");
		m_passRecording = false;
		m_currentPassType = PassType::None;
		EndGPUMarker();
	}

	void D3D12CommandList::EndRenderPass()
	{
		BF_PROFILE_EVENT();
		BF_CORE_ASSERT(m_passRecording, "D3D12CommandList::EndRenderPass called without a matching StartRenderPass.");
		BF_CORE_ASSERT(m_currentPassType == PassType::Raster, "D3D12CommandList::EndRenderPass called without a matching StartRenderPass.");

		m_passRecording = false;
		m_currentPassType = PassType::None;

		// Reset bound render targets and depth stencil to null to avoid accidental usage outside of a render pass.
		m_colorTexture.clear();
		m_colorTextureView.clear();
		m_depthStencilTexture = nullptr;
		m_depthStencilView = {};

		EndGPUMarker();
	}

	void D3D12CommandList::SetGraphicsPSO(const BFGraphicsPSOInfo& pso)
	{
		BF_PROFILE_EVENT();

		BF_CORE_ASSERT(m_colorTexture.empty() && !pso.PixelShader || !m_colorTexture.empty() && pso.PixelShader, "D3D12CommandList::SetGraphicsPSO needs to have a PixelShader.");
		BF_CORE_ASSERT(m_currentPassType == PassType::Raster, "D3D12CommandList::SetGraphicsPSO needs to be a raster pass.");

		PipelineStateStream pss;

		pss.RootSignature = D3D12API()->BindlessRootSignature();

		uint64_t psoHash = 0;

		// Set rendertarget formats.
		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = static_cast<uint32_t>(m_colorTexture.size());
		rtvFormats.RTFormats[0] = DXGI_FORMAT_UNKNOWN;
		for (size_t i = 0; i < m_colorTexture.size(); i++)
		{
			rtvFormats.RTFormats[i] = m_colorTexture[i]->Format();
			Utils::SumHash(psoHash, static_cast<uint64_t>(m_colorTexture[i]->Format()));
		}
		pss.RTVFormats = rtvFormats;

		// Set depth stencil format.
		if (m_depthStencilTexture)
		{
			pss.DSVFormat = m_depthStencilTexture->Format();
			Utils::SumHash(psoHash, static_cast<uint64_t>(m_depthStencilTexture->Format()));
		}

		// Set pixel shader.
		if (pso.PixelShader)
		{
			BF_CORE_ASSERT(pso.PixelShader->Type() == ShaderType::Pixel, "D3D12CommandList::SetGraphicsPSO Shader is not a Pixel shader.");
			pss.PS = { pso.PixelShader->Blob()->GetBufferPointer(), pso.PixelShader->Blob()->GetBufferSize() };
			Utils::SumHash(psoHash, static_cast<uint64_t>(pso.PixelShader->NumBytes()));
		}

		// Set vertex shader.
		if (pso.VertexShader)
		{
			BF_CORE_ASSERT(pso.VertexShader->Type() == ShaderType::Vertex, "D3D12CommandList::SetGraphicsPSO Shader is not a Vertex shader.");
			pss.VS = { pso.VertexShader->Blob()->GetBufferPointer(), pso.VertexShader->Blob()->GetBufferSize() };
			Utils::SumHash(psoHash, static_cast<uint64_t>(pso.VertexShader->NumBytes()));
		}

		// Set rasterizer.
		CD3DX12_RASTERIZER_DESC rasterizerDesc(pss.RasterizerState);
		rasterizerDesc.CullMode = pso.Rasterizer.CullMode;
		Utils::SumHash(psoHash, static_cast<uint64_t>(pso.Rasterizer.CullMode));
		pss.RasterizerState = rasterizerDesc;

		// Set DepthStencil state.
		CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(pss.DepthStencilState);
		depthStencilDesc.DepthWriteMask = pso.DepthStencil.WriteMask;
		depthStencilDesc.DepthFunc = pso.DepthStencil.DepthFunc;
		depthStencilDesc.DepthEnable = pso.DepthStencil.EnableDepth && m_depthStencilTexture;
		if(pso.DepthStencil.EnableDepth && !m_depthStencilTexture)
		{
			BF_CORE_LOG_WARN("D3D12CommandList::SetGraphicsPSO DepthStencil.EnableDepth is true but no DSV is bound.");
		}
		Utils::SumHash(psoHash, static_cast<uint64_t>(pso.DepthStencil.WriteMask));
		Utils::SumHash(psoHash, static_cast<uint64_t>(pso.DepthStencil.DepthFunc));
		Utils::SumHash(psoHash, static_cast<uint64_t>(pso.DepthStencil.EnableDepth));
		pss.DepthStencilState = depthStencilDesc;

		// Topology Type.
		pss.PrimitiveTopologyType = pso.PrimitiveTopologyType;
		m_cmdList->IASetPrimitiveTopology(Utils::GetPrimitiveTopologyFromType(pso.PrimitiveTopologyType));
		Utils::SumHash(psoHash, static_cast<uint64_t>(pso.PrimitiveTopologyType));

		// Set blend state.
		CD3DX12_BLEND_DESC blendDesc(pss.BlendDesc);

		for (uint32_t i = 0; i < 8; i++)
		{
			D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc;
			rtBlendDesc.BlendEnable = pso.DepthStencil.BlendStates[i].EnableBlending;
			rtBlendDesc.LogicOpEnable = FALSE;

			rtBlendDesc.SrcBlend = pso.DepthStencil.BlendStates[i].SrcBlend;
			rtBlendDesc.DestBlend = pso.DepthStencil.BlendStates[i].DestBlend;
			rtBlendDesc.BlendOp = pso.DepthStencil.BlendStates[i].BlendOp;

			rtBlendDesc.SrcBlendAlpha = pso.DepthStencil.BlendStates[i].SrcBlendAlpha;
			rtBlendDesc.DestBlendAlpha = pso.DepthStencil.BlendStates[i].DestBlendAlpha;
			rtBlendDesc.BlendOpAlpha = pso.DepthStencil.BlendStates[i].BlendOpAlpha;

			rtBlendDesc.RenderTargetWriteMask = pso.DepthStencil.BlendStates[i].RenderTargetWriteMask;

			blendDesc.RenderTarget[i] = rtBlendDesc;
			Utils::SumHash(psoHash, static_cast<uint64_t>(rtBlendDesc.BlendEnable));
			Utils::SumHash(psoHash, static_cast<uint64_t>(rtBlendDesc.SrcBlend));
		}

		pss.BlendDesc = blendDesc;

		const D3D12Pipeline& pipeline = BFPipelineCache::GetOrCreatePipeline(psoHash, &pss);
		m_cmdList->SetPipelineState(pipeline.GetHW());

	}

	void D3D12CommandList::SetComputePSO(const BFComputePSOInfo& pso)
	{
		BF_PROFILE_EVENT();

		BF_CORE_ASSERT(m_currentPassType == PassType::Compute, "D3D12CommandList::SetComputePSO can only be called during a compute pass.");
		BF_CORE_ASSERT(pso.ComputeShader, "D3D12CommandList::SetComputePSO needs to have a ComputeShader.");

		ComputePipelineStateStream pss;
		pss.RootSignature = D3D12API()->BindlessRootSignature();
		uint64_t psoHash = 0;

		// Set compute shader.
		BF_CORE_ASSERT(pso.ComputeShader->Type() == ShaderType::Compute, "D3D12CommandList::SetComputePSO Shader is not a Compute shader.");
		pss.CS = { pso.ComputeShader->Blob()->GetBufferPointer(), pso.ComputeShader->Blob()->GetBufferSize() };
		Utils::SumHash(psoHash, static_cast<uint64_t>(pso.ComputeShader->NumBytes()));

		const D3D12Pipeline& pipeline = BFPipelineCache::GetOrCreatePipeline(psoHash, &pss);
		m_cmdList->SetPipelineState(pipeline.GetHW());
	}

	void D3D12CommandList::SetIndexBuffer(const BFIndexBuffer& buffer)
	{
		BF_PROFILE_EVENT();

		m_cmdList->IASetIndexBuffer(&buffer.IBV());
	}
	
	void D3D12CommandList::SetViewport(const RenderViewport& viewport)
	{
		D3D12_VIEWPORT vp;
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = static_cast<FLOAT>(viewport.Width);
		vp.Height = static_cast<FLOAT>(viewport.Height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;

		D3D12_RECT scissorRect = { 0u, 0u, static_cast<LONG>(viewport.Width), static_cast<LONG>(viewport.Height) };
		m_cmdList->RSSetScissorRects(1, &scissorRect);
		m_cmdList->RSSetViewports(1, &vp);
	}

	void D3D12CommandList::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation)
	{
		BF_CORE_ASSERT(m_bindlessBound, "D3D12CommandList::DrawIndexedInstanced Bindless descriptor heaps and root signature must be set before drawing. Call StartRenderPass before drawing.");

		m_cmdList->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}

	void D3D12CommandList::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
	{
		BF_CORE_ASSERT(m_bindlessBound, "D3D12CommandList::DrawInstanced Bindless descriptor heaps and root signature must be set before drawing. Call StartRenderPass before drawing.");

		m_cmdList->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
	}

	void D3D12CommandList::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
	{
		BF_CORE_ASSERT(m_bindlessBound, "D3D12CommandList::Dispatch Bindless descriptor heaps and root signature must be set before dispatching. Call StartComputePass before dispatching.");
		m_cmdList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}

	void D3D12CommandList::EndGPUMarker()
	{
		m_cmdList->EndEvent();
	}
}
