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

        const std::filesystem::path brdfLUTPath = FileSystem::EngineAssetDirectory() / "LUT/BRDFLUT.bin";
        const std::vector<uint8_t> pixels = FileSystem::ReadBinary(brdfLUTPath);

		TextureDesc brdfLUTDesc;
		brdfLUTDesc.Width = 256;
		brdfLUTDesc.Height = 256;
		brdfLUTDesc.DebugName = "BRDFLUT";
		brdfLUTDesc.ViewTypes = TextureDesc::ViewType::ShaderResource;
		brdfLUTDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
		brdfLUTDesc.NumMips = 1;

        m_brdfLUT = MakeRef<Texture>(brdfLUTDesc);
		GraphicsCommands::UploadTextureResource(*m_brdfLUT, pixels, false);
    }

    SharedRenderResources::~SharedRenderResources()
    {
        m_whiteTexture.reset();
        m_brdfLUT.reset();
    }
}