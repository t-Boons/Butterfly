#pragma once
#include "D3D12Common.hpp"

namespace Butterfly
{
	enum class ShaderType
	{
		Vertex,
		Pixel,
		Compute,
	};

	struct ShaderMacro
	{
		ShaderMacro(const std::string& name, const std::string& value)
			: Name(name), Value(value)
		{}

		ShaderMacro(const std::string& name)
			: Name(name), Value("1")
		{}

		ShaderMacro()
			: Name(""), Value("1")
		{}

		std::string Name;
		std::string Value;
	};

	class Shader : private NonCopyable
	{
	public:
		const ComPtr<IDxcBlob> Blob() const { return m_shaderBlob; }

		ShaderType Type() const { return m_type; }
		uint32_t NumBytes() const { return m_numBytes; }

	private:
		Shader(const std::filesystem::path& filePath, const std::vector<ShaderMacro>& macros = {});

		friend class ShaderCache;
		friend class PipelineCache;
		ShaderType m_type;
		ComPtr<IDxcBlob> m_shaderBlob;
		uint32_t m_numBytes;

		inline static std::filesystem::path s_shaderDirectory{};
		inline static std::vector<std::wstring> s_includeDirectories{};
	};

	class ShaderCache
	{
	public:
		static Shader* GetOrCreate(const std::string& filePath, const std::vector<ShaderMacro>& macros = {});

	private:
		inline static std::unordered_map<uint64_t, ScopePtr<Shader>> m_map;
	};
}
