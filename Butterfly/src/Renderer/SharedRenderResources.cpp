#include "Renderer/SharedRenderResources.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/D3D12/D3D12GraphicsCommands.hpp"
#include "stbimage/stb_image.h"
#include "Core/FileSystem.hpp"

namespace Butterfly
{
    SharedRenderResources::SharedRenderResources()
    {
        TextureDesc whiteTextureDesc;
        whiteTextureDesc.Width = 1;
        whiteTextureDesc.Height = 1;
        whiteTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        whiteTextureDesc.DebugName = "WhiteTexture";
        whiteTextureDesc.ViewTypes = TextureDesc::ViewType::ShaderResource;
        whiteTextureDesc.NumMips = 1;
        m_whiteTexture = MakeRef<Texture>(whiteTextureDesc);


		int channels, width, height = 0;
        const std::filesystem::path brdfLUTPath = FileSystem::EngineAssetDirectory() / "LUT/BRDFLUT.png";
		stbi_uc* pixels = stbi_load(brdfLUTPath.string().c_str(), &width, &height, &channels, 4);
        
		TextureDesc brdfLUTDesc;
		brdfLUTDesc.Width = static_cast<uint32_t>(width);
		brdfLUTDesc.Height = static_cast<uint32_t>(height);
		brdfLUTDesc.DebugName = "BRDFLUT";
		brdfLUTDesc.ViewTypes = TextureDesc::ViewType::ShaderResource;
		brdfLUTDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		brdfLUTDesc.NumMips = 1;

        m_brdfLUT = MakeRef<Texture>(brdfLUTDesc);
		GraphicsCommands::UploadTextureResource(*m_brdfLUT, pixels);
        
		stbi_image_free(pixels);
    }

    SharedRenderResources::~SharedRenderResources()
    {
        m_whiteTexture.reset();
        m_brdfLUT.reset();
    }
}