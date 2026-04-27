#include "d3d12shadercompiler.hpp"

#include "interface/shader.hpp"
#include "d3d12common.hpp"

#include <engine/core/utility/utility.h>

#include <dxcapi.h>
#include <wrl/client.h>

#include <Windows.h>

#include <d3d12shader.h>

#include <vector>
#include <filesystem>
#include <fstream>


static constexpr const char* kShaderDirectory = "C:/Projects/BorealEngine/engine/shaders";


namespace render {

static const wchar_t* translateShaderTypeToDxcTargetProfile(EShaderType type)
{
	switch (type)
	{
		case EShaderType::kVertex:        return L"vs_6_6";
		case EShaderType::kPixel:         return L"ps_6_6";
		case EShaderType::kDomain:        return L"ds_6_6";
		case EShaderType::kHull:          return L"hs_6_6";
		case EShaderType::kGeometry:      return L"gs_6_6";
		case EShaderType::kCompute:       return L"cs_6_6";
		case EShaderType::kAmplification: return L"as_6_6";
		case EShaderType::kMesh:          return L"ms_6_6";
		default:                          return L"";
	}

}

// Helper to get error message from compile result
static std::string GetDxcErrorMessage(IDxcResult* result)
{
	if (result)
	{
		Microsoft::WRL::ComPtr<IDxcBlobEncoding> error_blob;
		if (SUCCEEDED(result->GetErrorBuffer(&error_blob)) && error_blob)
		{
			return std::string(static_cast<const char*>(error_blob->GetBufferPointer()), error_blob->GetBufferSize());
		}
	}
	return "Unknown compilation error";
}


static std::string loadShaderSourceFromFile(const std::string& source)
{
	std::filesystem::path shader_path = std::filesystem::path(kShaderDirectory) / std::string(source + ".hlsl");
	std::ifstream shader_file(shader_path, std::ios::binary);
	ASSERT_MSG_RETURN(shader_file.is_open(), std::string{},
		L"Failed to open shader source file. \nPath: %s", std::filesystem::absolute(shader_path).c_str());

	shader_file.seekg(0, std::ios::end);
	size_t file_size = shader_file.tellg();
	shader_file.seekg(0, std::ios::beg);
	std::string shader_source(file_size, '\0');
	shader_file.read(&shader_source[0], file_size);
	if (!shader_file)
	{
		throw std::runtime_error("Failed to read shader file: " + shader_path.string());
	}
	return shader_source;
}


class IncludeHandler : public IDxcIncludeHandler
{
public:
	ULONG STDMETHODCALLTYPE AddRef() override
	{
		return ++counter;
	}

	ULONG STDMETHODCALLTYPE Release() override
	{
		return --counter;
	}

	IncludeHandler(IDxcUtils* dxcUtils)
		: mDxcUtils(dxcUtils)
	{
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject) override
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_ IDxcBlob** ppIncludeSource) override
	{
		size_t file_name_len = wcslen(pFilename);
		size_t dummy = 0;
		std::string file_name;
		size_t file_name_capacity = file_name_len * 4;
		file_name.resize(file_name_capacity);

		wcstombs_s(&dummy, (char*)file_name.c_str(), file_name_capacity, pFilename, file_name_len);

		std::string shader_source = loadShaderSourceFromFile(file_name);
		if (shader_source.empty())
		{
			return E_FAIL;
		}

		HRESULT hr = mDxcUtils->CreateBlob(shader_source.data(), (UINT32)shader_source.size(), CP_UTF8, (IDxcBlobEncoding**)ppIncludeSource);

		return hr;
	}
private:
	IDxcUtils* mDxcUtils = nullptr;
	ULONG counter = 0;
};

std::unique_ptr<D3D12ShaderCompiler> D3D12ShaderCompiler::create(bool generate_debug_info)
{
	HMODULE lib = nullptr;
	ASSERT_MSG_RETURN_NULL(lib = LoadLibraryW(L"dxcompiler.dll"),
		L"Failed to load dxcompiler.dll. Message: %s", utility::getWinApiLastErrorString().c_str());

	struct LibGuard
	{
		HMODULE lib_ = nullptr;
		LibGuard(HMODULE& lib) : lib_(lib) {}
		~LibGuard() { if (lib_) FreeLibrary(lib_); }
	} guard(lib);

	DxcCreateInstanceProc dxc_create_instance_func = (DxcCreateInstanceProc)GetProcAddress(lib, "DxcCreateInstance");
	ASSERT_MSG_RETURN_NULL(dxc_create_instance_func, "Failed to get DxcCreateInstance function address");

	Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;
	D3D12_ASSERT_RETURN_NULL(dxc_create_instance_func(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

	Microsoft::WRL::ComPtr<IDxcUtils> utils;
	D3D12_ASSERT_RETURN_NULL(dxc_create_instance_func(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));

	guard.lib_ = nullptr;  // Release guard ownership since we have successfully created the compiler and utils

	return std::unique_ptr<D3D12ShaderCompiler>(
		new D3D12ShaderCompiler(compiler.Detach(), utils.Detach(), lib, generate_debug_info));
}


std::unique_ptr<IShader> D3D12ShaderCompiler::compile(
	EShaderType type,
	const std::string& source,
	std::wstring_view entryPoint, 
	std::span<const ShaderDefine> defines) const
{
	std::unique_ptr<IShader> result;

	ASSERT_MSG_RETURN(!source.empty(), result, "Shader source name cannot be empty");
	ASSERT_MSG_RETURN(!entryPoint.empty(), result, "Shader entry point cannot be empty");

	// Build compiler arguments
	std::vector<LPCWSTR> arguments;
	std::vector<std::wstring> wide_strings;  // Keep references alive

	// Entry point
	arguments.push_back(L"-E");
	arguments.push_back(entryPoint.data());

	arguments.push_back(L"-T");
	arguments.push_back(translateShaderTypeToDxcTargetProfile(type));

	arguments.push_back(L"-HV 2021");

	if (generate_debug_info_)
	{
		arguments.push_back(DXC_ARG_DEBUG);  // Enable debug information
		arguments.push_back(L"-Qembed_debug");  // Embed debug information in the shader bytecode
		arguments.push_back(L"-Od");  // Disable optimizations for better debugging experience
	}
	else
	{
		arguments.push_back(L"-O3");
	}

	// Defines
	std::vector<DxcDefine> dxc_defines;
	for (const auto& define : defines)
	{
		wide_strings.push_back(define.name + L"=" + define.value);
		arguments.push_back(L"-D");
		arguments.push_back(wide_strings.back().c_str());
	}

	std::string shader_source = loadShaderSourceFromFile(source);
	ASSERT_MSG_RETURN(!shader_source.empty(), result, "Failed to load shader source file: %s", source.c_str());

	DxcBuffer dxc_shader_source = {};
	dxc_shader_source.Size = shader_source.size();
	dxc_shader_source.Ptr = shader_source.data();
	dxc_shader_source.Encoding = DXC_CP_UTF8;

	IncludeHandler includeHandler(utils_);

	Microsoft::WRL::ComPtr<IDxcResult> compile_result;
	D3D12_ASSERT_RETURN_NULL(compiler_->Compile(
		&dxc_shader_source,
		arguments.data(),
		static_cast<UINT32>(arguments.size()),
		&includeHandler,
		IID_PPV_ARGS(&compile_result)));

	// Check compilation status
	HRESULT compile_status;
	D3D12_ASSERT_RETURN_NULL(compile_result->GetStatus(&compile_status));
	ASSERT_MSG_RETURN(SUCCEEDED(compile_status), result, "Compile error: %s", GetDxcErrorMessage(compile_result.Get()).c_str());

	// Get compiled object
	Microsoft::WRL::ComPtr<IDxcBlob> bytecode;
	D3D12_ASSERT_RETURN_NULL(compile_result->GetResult(&bytecode));

	const uint8_t* data = static_cast<const uint8_t*>(bytecode->GetBufferPointer());
	size_t size = bytecode->GetBufferSize();
	ASSERT_MSG_RETURN(data && size > 0, result, "Shader compilation produced empty bytecode for: %s", source.c_str());

	std::unique_ptr<D3D12ShaderReflection> reflection = D3D12ShaderReflection::create(utils_, compile_result.Get());

	return std::unique_ptr<IShader>(new IShader(std::vector<uint8_t>(data, data + size), std::move(reflection)));
}


D3D12ShaderCompiler::~D3D12ShaderCompiler()
{
	D3D12_SAFE_RELEASE(compiler_);
	D3D12_SAFE_RELEASE(utils_);
}


D3D12ShaderCompiler::D3D12ShaderCompiler(IDxcCompiler3* compiler, IDxcUtils* utils, HMODULE lib, bool generate_debug_info)
	:IShaderCompiler(generate_debug_info)
	, compiler_(compiler)
	, utils_(utils)
	, lib_(lib)
{
}


std::unique_ptr<D3D12ShaderReflection> D3D12ShaderReflection::create(IDxcUtils* utils, IDxcResult* compile_result)
{
	Microsoft::WRL::ComPtr<IDxcBlob> reflection_blob;
	Microsoft::WRL::ComPtr<IDxcBlobUtf16> reflection_name;
	compile_result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflection_blob), &reflection_name);

	if (reflection_blob)
	{
		DxcBuffer reflection_buffer;
		reflection_buffer.Ptr = reflection_blob->GetBufferPointer();
		reflection_buffer.Size = reflection_blob->GetBufferSize();
		reflection_buffer.Encoding = 0;

		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> shader_reflection;
		utils->CreateReflection(&reflection_buffer, IID_PPV_ARGS(&shader_reflection));

		D3D12_SHADER_DESC shader_desc;
		shader_reflection->GetDesc(&shader_desc);

		// Input parameters
		std::vector<D3D12_SIGNATURE_PARAMETER_DESC> input_parameters(shader_desc.InputParameters);
		for (UINT i = 0; i < shader_desc.InputParameters; i++)
		{
			D3D12_SIGNATURE_PARAMETER_DESC param;
			shader_reflection->GetInputParameterDesc(i, &param);
			input_parameters.push_back(param);
		}

		// Constant buffers
		std::vector<D3D12_SHADER_BUFFER_DESC> constant_buffers(shader_desc.ConstantBuffers);
		for (UINT i = 0; i < shader_desc.ConstantBuffers; i++)
		{
			ID3D12ShaderReflectionConstantBuffer* cb = shader_reflection->GetConstantBufferByIndex(i);
			D3D12_SHADER_BUFFER_DESC cb_desc;
			cb->GetDesc(&cb_desc);
			constant_buffers.push_back(cb_desc);
		}

		// Bound resources (textures, samplers, UAVs, etc.)
		std::vector<D3D12_SHADER_INPUT_BIND_DESC> resource_bindings(shader_desc.BoundResources);
		for (UINT i = 0; i < shader_desc.BoundResources; i++)
		{
			D3D12_SHADER_INPUT_BIND_DESC bind_desc;
			shader_reflection->GetResourceBindingDesc(i, &bind_desc);
			resource_bindings.push_back(bind_desc);
		}

		uint32_t thread_group_x = 0, thread_group_y = 0, thread_group_z = 0;
		shader_reflection->GetThreadGroupSize(&thread_group_x, &thread_group_y, &thread_group_z);
	}

	return std::unique_ptr<D3D12ShaderReflection>();
}

}  // namespace render