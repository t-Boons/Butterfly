#pragma once
#include "Butterfly.hpp"
#include "imgui/imgui.h"
#include "stbimage/stb_image.h"
#include "stbimage/stb_image_write.h"
#include "stbimage/stb_image_resize2.h"
#include "EditorCache/EditorCache.hpp"

namespace Butterfly
{
	class Thumbnail : public NonCopyableNonMoveable
	{
	public:
		uint32_t Width() const { return m_width; }
		uint32_t Height() const { return m_height; }

		void ValidateGPUTexture();

		ImTextureID GetImGUITextureID();

	private:
		friend class ThumbnailProcessor;

		Thumbnail() = default;

		std::vector<uint8_t> m_pixels;
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		std::string m_debugName = "Thumbnail";

		RefPtr<BFTexture> m_gpuTexture;
	};

	class ThumbnailProcessor
	{
	public:
		static RefPtr<Thumbnail> GetThumbnailFromBytes(const std::vector<uint8_t>& bytes);
		static std::vector<uint8_t> GetBytesFromThumbnail(const RefPtr<Thumbnail> thumbnail);
		static RefPtr<Thumbnail> Resize(const Thumbnail& input, uint32_t width, uint32_t height);
		static RefPtr<Thumbnail> GetThumbnailFromFile(const std::filesystem::path& filePath);
		static bool IsSupportedImageType(const std::filesystem::path& filePath);
	};

	class ThumbnailCacheEntry : public IEditorCacheEntry
	{
	public:
		ThumbnailCacheEntry(RefPtr<Thumbnail> thumbnail)
			: m_thumbnail(thumbnail)
		{
		}

		RefPtr<Thumbnail> GetThumbnail() const { return m_thumbnail; }

		ThumbnailCacheEntry() = default;

		std::vector<uint8_t> Serialize() const
		{
			return ThumbnailProcessor::GetBytesFromThumbnail(m_thumbnail);
		}

		void Deserialize(const std::vector<uint8_t>& data)
		{
			m_thumbnail = ThumbnailProcessor::GetThumbnailFromBytes(data);
		}

	private:
		RefPtr<Thumbnail> m_thumbnail;
	};
}