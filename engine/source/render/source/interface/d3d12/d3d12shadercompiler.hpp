#pragma once

#include "shadercompiler.hpp"

#include <string>
#include <memory>
#include <vector>


struct IDxcCompiler3;
struct IDxcUtils;
struct IDxcValidator;
struct IDxcResult;


namespace render {

class IShader;


class D3D12ShaderReflection final : public IShaderReflection
{
public:
	static std::unique_ptr<D3D12ShaderReflection> create(IDxcUtils* utils, IDxcResult* compile_result);

	~D3D12ShaderReflection() override = default;
};

class D3D12ShaderCompiler final : public IShaderCompiler
{
public:
	static std::unique_ptr<D3D12ShaderCompiler> create(bool generate_debug_info);

	std::unique_ptr<IShader> compile(
		EShaderType type,
		const std::string& source,
		std::wstring_view entryPoint,
		std::span<const ShaderDefine> defines) const override;

	~D3D12ShaderCompiler() override;

private:
	D3D12ShaderCompiler(IDxcCompiler3* compiler, IDxcUtils* utils, HMODULE lib, bool generate_debug_info);

private:
	HMODULE lib_;
	IDxcCompiler3* compiler_;
	IDxcUtils* utils_;
};

}  // namespace render
