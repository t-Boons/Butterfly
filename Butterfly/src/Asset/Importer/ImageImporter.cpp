#include "Asset/Importer/ImageImporter.hpp"
#include "Asset/AssetTypes.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "stbimage/stb_image.h"

namespace Butterfly
{
	BF_REGISTER_ASSET_IMPORTER(ImageImporter);

	bool ImageImporter::CanImport(const std::string& fileExtention) const
	{
		std::string extension = fileExtention;
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		return extension == ".jpg" ||
			   extension == ".jpeg"||
			   extension == ".png" ||
			   extension == ".tga" ||
			   extension == ".bmp" ||
			   extension == ".psd" ||
			   extension == ".hdr";
	}

	bool ImageImporter::CanImportType(const AssetType& type) const
	{
		return type == TextureAsset::Type;
	}

	bool ImageImporter::Import(const AssetFileMetadata& meta, AssetManager& manager) const
	{
		BFTextureDesc textureDesc;
		int channels, width, height = 0;

		stbi_uc* pixels = stbi_load(meta.Path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if (!pixels)
		{
			const char* reason = stbi_failure_reason();
			BF_CORE_LOG_ERROR("Failed to load image: %s", reason);
			return false;
		}

		textureDesc.Width = static_cast<uint32_t>(width);
		textureDesc.Height = static_cast<uint32_t>(height);
		textureDesc.DebugName = std::filesystem::path(meta.Path).filename().string();
		textureDesc.Flags = BFTextureDesc::Flag::ShaderResource;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Data = pixels;

		RefPtr<TextureAsset> textureAsset = MakeRef<TextureAsset>();
		textureAsset->Texture = BFTexture::CreateTextureFromCPUBuffer(textureDesc);

		manager.AddAssetEntry<TextureAsset>(AssetEntry{ meta.RootAssetID, TextureAsset::Type, StaticCastRef<void>(textureAsset) });

		stbi_image_free(pixels);
		return true;
	}

	bool ImageImporter::CreateMeta(const std::filesystem::path& file, AssetFileMetadata& meta) const
	{
		meta.SourceFileID = UUID::Generate();
		meta.Path = file;

		const auto uuid = UUID::Generate();
		meta.RootAssetID = uuid;
		meta.Assets[uuid] = AssetMetadata{ meta.SourceFileID, file.filename().string(), TextureAsset::Type, uuid };

		meta.SyncSourceFileIDWithAssets();
		return true;
	}
}