#include "Renderer/D3D12/D3D12Shader.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"

namespace Butterfly
{
	namespace Utils
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

		inline ShaderType ShaderTypeFromFilePath(const std::string& filePath)
		{
			BF_PROFILE_EVENT();
			const std::filesystem::path path(filePath);
			const std::string extension = path.extension().string();
			if (extension == ".hlsl")
			{
				if (filePath.find("_vert") != std::string::npos)
					return ShaderType::Vertex;
				else if (filePath.find("_frag") != std::string::npos)
					return ShaderType::Pixel;
				else if (filePath.find("_cs") != std::string::npos)
					return ShaderType::Compute;
			}
			BF_CORE_LOG_CRITICAL("Invalid shader file path: %s", filePath.c_str());
			return ShaderType::Vertex;
		}
	}

	BFShader::BFShader(const std::string& filePath, const std::vector<BFShaderMacro>& macros)
		: m_type(Utils::ShaderTypeFromFilePath(filePath))
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
				L"-I",
				includeDirectory.c_str()
			};

#endif	
			std::vector<DxcDefine> defines(macros.size());
			std::vector<std::wstring> macroNames(macros.size());
			std::vector<std::wstring> macroValues(macros.size());
			for (uint32_t i = 0; i < macros.size(); ++i)
			{
				macroNames[i] = Utils::StringToWString(macros[i].Name);
				macroValues[i] = Utils::StringToWString(macros[i].Value);
			}

			for (uint32_t i = 0; i < macros.size(); ++i)
			{
				DxcDefine define;
				define.Name = macroNames[i].c_str();
				define.Value = macroValues[i].c_str();
				defines[i] = define;
			}
				
			ThrowIfFailed(D3D12API()->ShaderUtils()->BuildArguments(
				Utils::StringToWString(filePath).c_str(),
				L"main",
				Utils::ShaderTypeToTargetProfile(m_type),
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
				std::stringstream ss;
				ss << "Shader compilation errors:\n" << errors->GetStringPointer();
				BF_CORE_LOG_ERROR("%s", ss.str().c_str());
				MessageBoxA(GetActiveWindow(), "Retry shader compilation?", "Shader compilation failed.", MB_OK);
				continue;
			}

			success = true;
		}

		ThrowIfFailed(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&m_shaderBlob), nullptr));

		m_numBytes = static_cast<uint32_t>(m_shaderBlob->GetBufferSize());
	}


	BFShader* BFShaderCache::GetOrCreate(const std::string& filePath, const std::vector<BFShaderMacro>& macros)
	{
		BF_PROFILE_EVENT();

		uint64_t hash = std::hash<std::string>()(filePath);
		for (auto& c : macros)
		{
			Utils::SumHash(hash, std::hash<std::string>()(c.Name));
			Utils::SumHash(hash, std::hash<std::string>()(c.Value));
		}

		auto shader = m_map.find(hash);
		if (shader == m_map.end())
		{
			BF_CORE_LOG_INFO("Created New Shader with filePath: %s", filePath.c_str());
			m_map[hash] = ScopePtr<BFShader>(new BFShader(filePath, macros));
			shader = m_map.find(hash);
		}

		return shader->second.get();
	}
}
