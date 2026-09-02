#include "Renderer/Graph/GraphTypes.hpp"
#include "Renderer/Texture_D3D12.hpp"

namespace Butterfly
{
	BFRGTexture::~BFRGTexture()
	{
		FREE(m_resource);
	}
}