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

	struct BFShaderMacro
	{
		BFShaderMacro(const std::string& name, const std::string& value)
			: Name(name), Value(value)
		{}

		BFShaderMacro(const std::string& name)
			: Name(name), Value("1")
		{}

		BFShaderMacro()
			: Name(""), Value("1")
		{}

		std::string Name;
		std::string Value;
	};

	class BFShader : private NonCopyable
	{
	public:
		const ComPtr<IDxcBlob> Blob() const { return m_shaderBlob; }

		ShaderType Type() const { return m_type; }
		uint32_t NumBytes() const { return m_numBytes; }

	private:
		BFShader(const std::string& filePath, const std::vector<BFShaderMacro>& macros = {});

		friend class BFShaderCache;
		friend class BFPipelineCache;
		ShaderType m_type;
		ComPtr<IDxcBlob> m_shaderBlob;
		uint32_t m_numBytes;
	};

	class BFShaderCache
	{
	public:
		static BFShader* GetOrCreate(const std::string& filePath, const std::vector<BFShaderMacro>& macros = {});

	private:
		inline static std::unordered_map<uint64_t, ScopePtr<BFShader>> m_map;
	};
}
