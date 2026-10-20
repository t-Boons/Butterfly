#include "Renderer/D3D12/D3D12Shader.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"

namespace Butterfly
{
	inline const wchar_t* ShaderTypeToTargetProfile(ShaderType type)
	{
		BF_PROFILE_EVENT();

		switch (type)
		{
		case ShaderType::Pixel: return L"ps_6_6";
		case ShaderType::Vertex: return L"vs_6_6";
		case ShaderType::Compute: return L"cs_6_6";
		}

		BF_CORE_LOG_CRITICAL("Invalid ShaderType given.");
		return nullptr;
	}

	BFShader::BFShader(const std::string& filePath, ShaderType type, const std::vector<std::string>& macros)
		: m_type(type)
	{
		BF_PROFILE_EVENT();

		ComPtr<IDxcResult> result;
		bool success = false;
		while (!success)
		{
			uint32_t codePage = CP_UTF8;
			ComPtr<IDxcBlobEncoding> sourceBlob;
			ThrowIfFailed(D3D12API()->ShaderUtils()->LoadFile(Utils::StringToWString(filePath).c_str(), &codePage, &sourceBlob));
			const std::filesystem::path shaderPath(filePath);
			const std::wstring includeDirectory = shaderPath.parent_path().wstring();


			ComPtr<IDxcCompilerArgs> args;
#ifdef BUTTERFLY_DEBUG
			LPCWSTR customArgs[] =
			{
				L"-all_resources_bound",
				L"-Zi",
				L"-Qembed_debug",
				L"-I",
				includeDirectory.c_str()
			};
#else
				LPCWSTR customArgs[] =
			{
				L"-all_resources_bound",
				L"-Zi",
				includeDirectory.c_str()
			};

#endif
			std::vector<DxcDefine> defines;
			for (const auto& macro : macros)
			{
				DxcDefine define;
				define.Name = Utils::StringToWString(macro).c_str();
				defines.push_back(define);
			}
				
			ThrowIfFailed(D3D12API()->ShaderUtils()->BuildArguments(
				Utils::StringToWString(filePath).c_str(),
				L"main",
				ShaderTypeToTargetProfile(type),
				customArgs,
				_countof(customArgs),
				defines.data(),
				defines.size(),
				&args));

			const DxcBuffer shaderBuffer = DxcBuffer{ sourceBlob->GetBufferPointer(), sourceBlob->GetBufferSize(), DXC_CP_ACP };

			IDxcIncludeHandler* includeHandler;
			ThrowIfFailed(D3D12API()->ShaderUtils()->CreateDefaultIncludeHandler(&includeHandler));

			ThrowIfFailed(D3D12API()->ShaderCompiler()->Compile(&shaderBuffer, args->GetArguments(), args->GetCount(), includeHandler, IID_PPV_ARGS(&result)));

			ComPtr<IDxcBlobUtf8> errors;
			ThrowIfFailed(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
			if (errors && errors->GetStringLength() > 0)
			{
#ifdef BUTTERFLY_DEBUG
				std::stringstream ss;
				ss << "Shader compilation errors:\n" << errors->GetStringPointer();
				BF_CORE_LOG_ERROR("%s", ss.str().c_str());
				MessageBoxA(GetActiveWindow(), "Retry shader compilation?", "Shader compilation failed.", MB_OK);
				continue;
#endif
			}

			success = true;
		}

		ThrowIfFailed(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&m_shaderBlob), nullptr));

		m_numBytes = static_cast<uint32_t>(m_shaderBlob->GetBufferSize());
	}


	BFShader* BFShaderCache::GetOrCreate(const std::string& filePath, ShaderType type, const std::vector<std::string>& macros)
	{
		BF_PROFILE_EVENT();

		uint64_t hash = std::hash<std::string>()(filePath);
		for (auto& c : macros)
		{
			Utils::SumHash(hash, std::hash<std::string>()(c));
		}

		auto shader = m_map.find(hash);
		if (shader == m_map.end())
		{
			BF_CORE_LOG_INFO("Created New Shader with filePath: %ls", filePath.c_str());
			m_map[hash] = ScopePtr<BFShader>(new BFShader(filePath, type, macros));
		}

		BF_CORE_ASSERT(m_map[hash]->Type() == type, "Shader type mismatch for filePath: %ls", filePath.c_str());
		return m_map[hash].get();
	}
}
