#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
    class Texture;

    class SharedRenderResources
    {
    public:
        SharedRenderResources();
        ~SharedRenderResources();

        RefPtr<Texture> GetWhiteTexture() const {return m_whiteTexture;};
        RefPtr<Texture> GetBRDFLUT() const {return m_brdfLUT;};

    private:
        RefPtr<Texture> m_whiteTexture;
        RefPtr<Texture> m_brdfLUT;
    };
}