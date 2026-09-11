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

	bool ImageImporter::CanImportType(const std::type_info& type) const
	{
		return type == typeid(TextureAsset);
	}

	bool ImageImporter::Import(const AssetMetadata& path, ImportResult& ret) const
	{
		BFTextureDesc textureDesc;
		int channels, width, height = 0;

		stbi_uc* pixels = stbi_load(path.Path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if (!pixels)
		{
			const char* reason = stbi_failure_reason();
			BF_CORE_LOG_ERROR("Failed to load image: %s", reason);
			return false;
		}

		textureDesc.Width = static_cast<uint32_t>(width);
		textureDesc.Height = static_cast<uint32_t>(height);
		textureDesc.DebugName = std::filesystem::path(path.Path).filename().string();
		textureDesc.Flags = BFTextureDesc::Flag::ShaderResource;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;


		RefPtr<TextureAsset> textureAsset = MakeRef<TextureAsset>();
		textureAsset->Texture = BFTexture::CreateTextureFromCPUBuffer(textureDesc, pixels);

		ret.Asset.Data = StaticCastRef<void>(textureAsset);
		ret.Asset.Type = AssetType{ "Texture" };

		stbi_image_free(pixels);
		return true;
	}
}