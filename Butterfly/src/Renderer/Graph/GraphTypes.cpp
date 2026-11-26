#include "Renderer/Graph/GraphTypes.hpp"
#include "Renderer/D3D12Texture.hpp"

namespace Butterfly
{
	RGTexture::~RGTexture()
	{
		m_resource.reset();
	}
}
