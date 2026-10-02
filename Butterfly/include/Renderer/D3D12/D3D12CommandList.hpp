#pragma once
#include "D3D12Common.hpp"
#include "Renderer/CommandList.hpp"

namespace Butterfly
{
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

		void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation);
		void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation);

		void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);

		ID3D12GraphicsCommandList* List() const { return m_cmdList; }
		D3D12_COMMAND_LIST_TYPE Type() const { return m_dxType; }

	private:
		bool m_cmdListClosed;
		bool m_hasExecuted;
		ID3D12GraphicsCommandList* m_cmdList;
		ID3D12CommandAllocator* m_allocator;
		D3D12_COMMAND_LIST_TYPE m_dxType;
	};
}
