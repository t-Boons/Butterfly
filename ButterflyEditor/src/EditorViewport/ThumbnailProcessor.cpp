#include "EditorViewport/ThumbnailProcessor.hpp"

namespace Butterfly
{
	void Thumbnail::ValidateGPUTexture()
	{
		if (m_gpuTexture)
		{
			return;
		}

		BFTextureDesc textureDesc;
		textureDesc.Width = m_width;
		textureDesc.Height = m_height;
		textureDesc.DebugName = m_debugName;
		textureDesc.Flags = BFTextureDesc::Flag::ShaderResource;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Data = m_pixels.data();
		m_gpuTexture = BFTexture::CreateTextureFromCPUBuffer(textureDesc);
	}

	ImTextureID Thumbnail::GetImGUITextureID()
	{
		ValidateGPUTexture();
		return (ImTextureID)(uintptr_t)D3D12API()->DescriptorAllocatorSrvCbvUav()->GpuHandleFromSrvHandle(m_gpuTexture->SRV().View()).ptr;
	}
}