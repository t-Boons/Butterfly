#include "Core/FileSystem.hpp"

namespace Butterfly
{
    bool FileSystem::Exists(const std::filesystem::path& path)
    {
        return std::filesystem::exists(path);
    }

    std::vector<uint8_t> FileSystem::ReadBinary(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);

        if (!file)
        {
            BF_CORE_LOG_WARN("Directory does not exist: %ls. Cannot read binary.", path.c_str());
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
            BF_CORE_LOG_WARN("Directory does not exist: %ls. Canot read text.", path.c_str());
            return {};
        }

        return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    }

    bool FileSystem::WriteBinary(const std::filesystem::path& path, const std::vector<uint8_t>& data)
    {
        std::ofstream file(path, std::ios::binary);
 
        if (!file)
        {
            BF_CORE_LOG_WARN("Directory does not exist: %ls. Cannot write binary.", path.c_str());
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

        return file.good();
    }

    bool FileSystem::Copy(const std::filesystem::path& source, const std::filesystem::path& dest)
    {
        std::ifstream src(source, std::ios::binary);

        if (!src.is_open())
        {
            BF_CORE_LOG_WARN("Failed to open source file: %ls", source.c_str());
            return false;
        }

        std::ofstream dst(dest, std::ios::binary | std::ios::trunc);

        if (!dst.is_open())
        {
            BF_CORE_LOG_WARN("Failed to open destination file: %ls",dest.c_str());
            return false;
        }

        dst << src.rdbuf();

        if (src.bad())
        {
            BF_CORE_LOG_WARN("Failed while reading source file: %ls", source.c_str());
            return false;
        }

        if (dst.fail())
        {
            BF_CORE_LOG_WARN("Failed while writing destination file: %ls", dest.c_str());
            return false;
        }

        dst.close();

        if (dst.fail())
        {
            BF_CORE_LOG_WARN("Failed to finalize destination file: %ls", dest.c_str());
            return false;
        }

        BF_CORE_LOG_TRACE(
            "Copied file: %ls to %ls",
            source.c_str(),
            dest.c_str()
        );

        return true;
    }

    bool FileSystem::WriteText(const std::filesystem::path& path, const std::string& data)
    {
        std::ofstream file(path);

        if (!file.is_open())
        {
            BF_CORE_LOG_WARN("Cannot open/create file: %ls", path.c_str());
            return false;
        }

        file.write(data.data(), static_cast<std::streamsize>(data.size()));

        BF_CORE_LOG_TRACE("FileSystem::WriteText -> %ls %u", path.c_str(), data.length());

        return file.good();
    }

    std::filesystem::path FileSystem::WorkingDirectory()
    {
        return std::filesystem::current_path();
    }

    std::filesystem::path FileSystem::ReplaceExtention(const std::filesystem::path& path, const std::string& extention)
    {
        return path.parent_path() / (path.stem().string() + extention);
    }

    std::vector<std::filesystem::path> FileSystem::WalkDirectoryRecursive(const std::filesystem::path& rootDir)
    {
        std::vector<std::filesystem::path> results;

        if (!std::filesystem::exists(rootDir) || !std::filesystem::is_directory(rootDir))
        {
            BF_CORE_LOG_ERROR("%ls' is not a valid directory.", rootDir.c_str());
            return results;
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(rootDir))
        {
            if (entry.is_regular_file())
            {
                results.push_back(entry.path());
            }
        }

        return results;
    }
}