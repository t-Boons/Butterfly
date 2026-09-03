#include "Renderer/Graph/GraphTypes.hpp"
#include "Renderer/D3D12Texture.hpp"

namespace Butterfly
{
	BFRGTexture::~BFRGTexture()
	{
		m_resource.reset();
	}
}
