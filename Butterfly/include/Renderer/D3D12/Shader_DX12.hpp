#pragma once
#include "Common_DX12.hpp"

namespace Butterfly
{
	enum class ShaderType
	{
		Vertex,
		Pixel,
		Compute,
	};

	class BFShader : private NonCopyable
	{
	public:
		const ComPtr<IDxcBlob> Blob() const { return m_shaderBlob; }

		ShaderType Type() const { return m_type; }
		uint32_t NumBytes() const { return m_numBytes; }

	private:
		BFShader(const std::wstring& filePath, ShaderType type);

		friend class BFShaderCache;
		friend class BFPipelineCache;
		ShaderType m_type;
		ComPtr<IDxcBlob> m_shaderBlob;
		uint32_t m_numBytes;
	};

	class BFShaderCache
	{
	public:
		static BFShader* GetOrCreate(const std::wstring& filePath, ShaderType type);

	private:
		inline static std::unordered_map<std::wstring, ScopePtr<BFShader>> m_map;
	};
}