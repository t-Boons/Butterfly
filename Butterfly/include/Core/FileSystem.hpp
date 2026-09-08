#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
	class FileSystem
	{
	public:
		static bool Exists(const std::filesystem::path& path);

		static std::vector<uint8_t> ReadBinary(const std::filesystem::path& path);

		static std::string ReadText(const std::filesystem::path& path);

		static bool WriteBinary(const std::filesystem::path& path, const std::vector<uint8_t>& data);

		static bool WriteText(const std::filesystem::path& path, const std::string& data);

		static bool Copy(const std::filesystem::path& source, const std::filesystem::path& dest);

		static std::filesystem::path ReplaceExtention(const std::filesystem::path& path, const std::string& extention);

		static std::filesystem::path WorkingDirectory();
	};
}