#include "Renderer/ModelLoading/ModelImporter.hpp"
#include "Renderer/ModelLoading/GLTF/ModelImporterGLTF.hpp"

namespace Butterfly
{
	RefPtr<ModelImporter> GetModelLoaderTypeFromFileType(const std::string& fileType)
	{
		if (fileType == ".gltf") return MakeRef<ModelImporterGltf>();

		Log::Critical("No valid loader found for fileType: %s", fileType.c_str());
		return nullptr;
	}

	RefPtr<ModelImporter> ModelImporter::Create(const std::string& filePath)
	{
		std::filesystem::path fp(filePath);

		Log::Assert(fp.has_extension(), "FilePath does not have an extention: %s", filePath.c_str());
		Log::Assert(fp.has_filename(), "FilePath does not have an name: %s", filePath.c_str());

		auto loader = GetModelLoaderTypeFromFileType(fp.extension().string());
		if (!loader) return nullptr;
		loader->m_filePath = filePath;
		return loader;
	}
}