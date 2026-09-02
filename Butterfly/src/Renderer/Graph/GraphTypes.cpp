#include "Renderer/Graph/GraphTypes.hpp"
#include "Renderer/Texture_DX12.hpp"

namespace Butterfly
{
	BFRGTexture::~BFRGTexture()
	{
		FREE(m_resource);
	}
}