#pragma once
#include "Core/Common.hpp"
#include "Renderer/D3D12Texture.hpp"

namespace Butterfly
{
	class RGResource;
	class Resource;

	class Texture;
	class StructuredBuffer;
	class ConstantBuffer;
	class IndexBuffer;
	class Sampler;


	// Resource Initializers.

	struct RGTextureInitializer
	{
	public:
		RGTextureInitializer(const TextureDesc& desc, const std::string& name)
		{
			Desc = desc;
			Name = name;
		}

		TextureDesc Desc;
		std::string Name;
	};


	class RGResource : private NonCopyable
	{
	public:
		virtual ~RGResource() = default;
	};

	class RGTexture : public RGResource
	{
	public:
		friend class GraphTransientResourceCache;

		RefPtr<Texture> Resource() const { return m_resource; }

		~RGTexture();

	protected:
		RGTexture(const TextureDesc& desc)
		{
			m_resource = MakeRef<Texture>(desc);
		}

		RefPtr<Texture> m_resource;
	};

	class RGStructuredBuffer : public RGResource
	{
	};

	class RGUniformBuffer : public RGResource
	{
	};

	class RGIndexBuffer : public RGResource
	{
	};

	class RGSampler : public RGResource
	{
	};
}
