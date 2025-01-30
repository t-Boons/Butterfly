#include "Renderer/Graph/GraphTypes.hpp"
#include "Renderer/Texture_DX12.hpp"

namespace Butterfly
{
	BFRGTexture::~BFRGTexture()
	{
		FREE(m_resource);
	}

	BFRGTexture::BFRGTexture(const BFRGTextureInitializer& initializer)
	{
		m_resource = BFTexture::CreateTextureForGPU(initializer.Desc, initializer.Name);
	}
}