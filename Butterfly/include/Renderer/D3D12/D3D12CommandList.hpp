#pragma once
#include "D3D12Common.hpp"
#include "Renderer/CommandList.hpp"

namespace Butterfly
{
	class BFTexture;
	struct BFGraphicsPSOInfo;
	class BFComputePSOInfo;
	class BFView;
	class BFIndexBuffer;

	enum class LoadOP
	{
		Load,
		Clear,
	};

	struct RenderViewport
	{
		static RenderViewport FromTexture(const BFTexture& texture);

		uint32_t Width = 1;
		uint32_t Height = 1;
	};

	struct RasterPassStartInfo
	{
		BFTexture* RenderTarget;
		LoadOP RenderTargetLoadOp = LoadOP::Load;
		glm::vec4 ClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		BFTexture* DepthStencil = nullptr;
		LoadOP DepthStencilLoadOp = LoadOP::Load;
		float DepthValue = 1.0f;
	};

	class D3D12CommandList : public CommandList
	{
	public:
		D3D12CommandList(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
		~D3D12CommandList();

		virtual void Reset() override;
		virtual void Close() override;
		virtual void Marker(const std::string& str) override;
		virtual void BeginGPUMarker(const std::string& str) override;
		virtual void EndGPUMarker() override;

		void StartRenderPass(const RasterPassStartInfo& info, const std::string& name);
		void StartComputePass(const std::string& name);
		void EndRenderPass();
		void EndComputePass();

		void SetGraphicsPSO(const BFGraphicsPSOInfo& pso);
		void SetComputePSO(const BFComputePSOInfo& pso);

		void SetIndexBuffer(const BFIndexBuffer& buffer);

		void SetViewport(const RenderViewport& viewport);

		void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation);
		void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation);

		void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);

		ID3D12GraphicsCommandList* List() const { return m_cmdList; }
		D3D12_COMMAND_LIST_TYPE Type() const { return m_dxType; }

	private:
		enum class PassType
		{
			None,
			Raster,
			Compute
		};

		bool m_cmdListClosed;
		bool m_hasExecuted;
		ID3D12GraphicsCommandList* m_cmdList;
		ID3D12CommandAllocator* m_allocator;
		D3D12_COMMAND_LIST_TYPE m_dxType;

		std::vector<BFTexture*> m_boundRts;
		BFTexture* m_boundDsv = nullptr;

		bool m_bindlessBound = false;
		PassType m_currentPassType = PassType::None;
		bool m_passRecording = false;
		std::string m_passName;
	};
}
