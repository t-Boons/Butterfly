#pragma once
#include "Core/Common.hpp"
#include "Renderer/Texture_DX12.hpp"

namespace Butterfly
{
	class BFRGResource;
	class BFResource;

	class BFTexture;
	class BFStructuredBuffer;
	class BFUniformBuffer;
	class BFIndexBuffer;
	class BFSampler;


	// Resource Initializers.

	struct BFRGTextureInitializer
	{
	public:
		BFRGTextureInitializer(const BFTextureDesc& desc, const std::string& name)
		{
			Desc = desc;
			Name = name;
		}

		BFTextureDesc Desc;
		std::string Name;
	};


	class BFRGResource : private NonCopyable
	{
	public:
		virtual ~BFRGResource() = default;
	};

	class BFRGTexture : public BFRGResource
	{
	public:
		friend class GraphTransientResourceCache;

		BFTexture* Resource() const { return m_resource; }

		~BFRGTexture();

	protected:
		BFRGTexture(const BFRGTextureInitializer& initializer);

		BFTexture* m_resource;
	};

	class BFRGStructuredBuffer : public BFRGResource
	{
	};

	class BFRGUniformBuffer : public BFRGResource
	{
	};

	class BFRGIndexBuffer : public BFRGResource
	{
	};

	class BFRGSampler : public BFRGResource
	{
	};
}