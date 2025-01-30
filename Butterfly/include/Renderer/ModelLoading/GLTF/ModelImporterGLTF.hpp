#pragma once
#include "Core/Common.hpp"
#include "Renderer/ModelLoading/ModelImporter.hpp"

namespace Butterfly
{
    class ModelImporterGltf : public ModelImporter
    {
    public:
        virtual void LoadImpl(const std::string& filePath) override;
    };
}