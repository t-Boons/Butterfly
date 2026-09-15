#pragma once
#include "Core/Common.hpp"
#include "Core/UUID.hpp"
#include <yaml-cpp/yaml.h>

namespace Butterfly
{
	struct AssetType
	{
		std::string TypeName;
	};

	struct AssetMetadata
	{
		std::string Path;
		std::string Extention;
		AssetType Type;
		UUID ID;
	};

	struct AssetEntry
	{
		UUID ID;
		AssetType Type;
		RefPtr<void> Data;
		uint32_t RefCount = 0;
	};
}

