#include "Renderer/D3D12/Shader_DX12.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"

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

		Log::Critical("Invalid ShaderType given.");
		return nullptr;
	}

	BFShader::BFShader(const std::wstring& filePath, ShaderType type)
		: m_type(type)
	{
		BF_PROFILE_EVENT();

		ComPtr<IDxcResult> result;
		bool success = false;
		while (!success)
		{
			uint32_t codePage = CP_UTF8;
			ComPtr<IDxcBlobEncoding> sourceBlob;
			ThrowIfFailed(DX12API()->ShaderUtils()->LoadFile(filePath.c_str(), &codePage, &sourceBlob));


			ComPtr<IDxcCompilerArgs> args;
			LPCWSTR customArgs[] = {
				L"-all_resources_bound",
				L"-Zi",
				L"-Qembed_debug"
			};

			ThrowIfFailed(DX12API()->ShaderUtils()->BuildArguments(
				filePath.c_str(),
				L"main",
				ShaderTypeToTargetProfile(type),
				customArgs,
				_countof(customArgs),
				nullptr,
				0,
				&args));

			const DxcBuffer shaderBuffer = DxcBuffer{ sourceBlob->GetBufferPointer(), sourceBlob->GetBufferSize(), DXC_CP_ACP };

			ThrowIfFailed(DX12API()->ShaderCompiler()->Compile(&shaderBuffer, args->GetArguments(), args->GetCount(), nullptr, IID_PPV_ARGS(&result)));

			ComPtr<IDxcBlobUtf8> errors;
			ThrowIfFailed(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
			if (errors && errors->GetStringLength() > 0)
			{
#ifdef BUTTERFLY_DEBUG
				std::stringstream ss;
				ss << "Shader compilation errors:\n" << errors->GetStringPointer();
				Log::Error("%s", ss.str().c_str());
				MessageBoxA(GetActiveWindow(), "Retry shader compilation?", "Shader compilation failed.", MB_OK);
				continue;
#endif
			}

			success = true;
		}

		ThrowIfFailed(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&m_shaderBlob), nullptr));

		m_numBytes = static_cast<uint32_t>(m_shaderBlob->GetBufferSize());
	}


	BFShader* BFShaderCache::GetOrCreate(const std::wstring& filePath, ShaderType type)
	{
		BF_PROFILE_EVENT();

		auto shader = m_map.find(filePath);
		if (shader == m_map.end())
		{
			Log::Info("Created New Shader with filePath: %ls", filePath.c_str());
			m_map[filePath] = ScopePtr<BFShader>(new BFShader(filePath, type));
		}

		Check(m_map[filePath]->Type() == type);
		return m_map[filePath].get();
	}
}