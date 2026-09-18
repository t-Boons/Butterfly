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


	RefPtr<Thumbnail> ThumbnailProcessor::GetThumbnailFromBytes(const std::vector<uint8_t>& bytes)
	{
		int channels, width, height = 0;
		stbi_uc* pixels = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width, &height, &channels, STBI_rgb_alpha);
		if (!pixels)
		{
			const char* reason = stbi_failure_reason();
			BF_CORE_LOG_ERROR("Failed to load thumbnail: %s", reason);
			return nullptr;
		}
		RefPtr<Thumbnail> thumbnail = RefPtr<Thumbnail>(new Thumbnail());
		thumbnail->m_pixels = std::vector<uint8_t>(pixels, pixels + (width * height * 4));
		thumbnail->m_width = static_cast<uint32_t>(width);
		thumbnail->m_height = static_cast<uint32_t>(height);
		stbi_image_free(pixels);
		return thumbnail;
	}

	 std::vector<uint8_t> ThumbnailProcessor::GetBytesFromThumbnail(const RefPtr<Thumbnail> thumbnail)
	{
		std::vector<uint8_t> bytes;
		stbi_write_png_to_func([](void* context, void* data, int size)
			{
				std::vector<uint8_t>* bytes = reinterpret_cast<std::vector<uint8_t>*>(context);
				bytes->insert(bytes->end(), reinterpret_cast<uint8_t*>(data), reinterpret_cast<uint8_t*>(data) + size);
			}, &bytes,
			static_cast<int>(thumbnail->Width()),
				static_cast<int>(thumbnail->Height()),
				STBI_rgb_alpha,
				thumbnail->m_pixels.data(),
				static_cast<int>(thumbnail->Width() * 4)
				);
		return bytes;
	}

	RefPtr<Thumbnail> ThumbnailProcessor::Resize(const Thumbnail& input, uint32_t width, uint32_t height)
	{
		RefPtr<Thumbnail> thumbnail = RefPtr<Thumbnail>(new Thumbnail());
		thumbnail->m_pixels.resize(width * height * 4);

		stbir_resize_uint8_linear(input.m_pixels.data(), input.m_width, input.m_height, 0, thumbnail->m_pixels.data(), width, height, 0, STBIR_RGBA);

		thumbnail->m_debugName = input.m_debugName;
		thumbnail->m_width = width;
		thumbnail->m_height = height;

		return thumbnail;
	}

	 RefPtr<Thumbnail> ThumbnailProcessor::GetThumbnailFromFile(const std::filesystem::path& filePath)
	{
		int channels, width, height = 0;

		stbi_uc* pixels = stbi_load(filePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if (!pixels)
		{
			const char* reason = stbi_failure_reason();
			BF_CORE_LOG_ERROR("Failed to load thumbnail: %s", reason);
			return nullptr;
		}


		RefPtr<Thumbnail> thumbnail = RefPtr<Thumbnail>(new Thumbnail());
		thumbnail->m_pixels = std::vector<uint8_t>(pixels, pixels + (width * height * 4));
		thumbnail->m_width = static_cast<uint32_t>(width);
		thumbnail->m_height = static_cast<uint32_t>(height);

		stbi_image_free(pixels);
		return thumbnail;
	}

	 bool ThumbnailProcessor::IsSupportedImageType(const std::filesystem::path& filePath)
	{
		std::string extension = filePath.extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

		return extension == ".jpg" ||
			extension == ".jpeg" ||
			extension == ".png" ||
			extension == ".tga" ||
			extension == ".bmp" ||
			extension == ".psd" ||
			extension == ".hdr";
	}
}