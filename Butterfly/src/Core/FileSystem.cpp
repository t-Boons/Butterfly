#include "Core/FileSystem.hpp"

namespace Butterfly
{
    bool FileSystem::Exists(const std::filesystem::path& path)
    {
        return std::filesystem::exists(path);
    }

    std::vector<uint8_t> FileSystem::ReadBinary(const std::filesystem::path& path)
    {
        const std::filesystem::path assetPath = s_assetPath / path;
        std::ifstream file(assetPath, std::ios::binary | std::ios::ate);

        if (!file)
        {
            BF_CORE_LOG_WARN("Directory does not exist: %s. Cannot read binary.", path.c_str());
            return {};
        }

        const auto size = file.tellg();

        std::vector<uint8_t> data(static_cast<size_t>(size));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));

        return data;
    }

    std::string FileSystem::ReadText(const std::filesystem::path& path)
    {
        std::ifstream file(path);   

        if (!file)
        {
            BF_CORE_LOG_WARN("Directory does not exist: %s. Canot read text.", path.c_str());
            return {};
        }

        return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    }

    bool FileSystem::WriteBinary(const std::filesystem::path& path, const std::vector<uint8_t>& data)
    {
        std::ofstream file(path, std::ios::binary);
 
        if (!file)
        {
            BF_CORE_LOG_WARN("Directory does not exist: %s. Cannot write binary.", path.c_str());
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

        return file.good();
    }

    bool FileSystem::WriteText(const std::filesystem::path& path, const std::string& data)
    {
        std::ofstream file(path);

        if (!file)
        {
            BF_CORE_LOG_WARN("Directory does not exist: %s. Cannot write text.", path.c_str());
            return false;
        }

        file.write(data.data(), static_cast<std::streamsize>(data.size()));

        return file.good();
    }
}