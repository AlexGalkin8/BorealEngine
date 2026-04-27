#pragma once

#include "interface/common.h"
#include "interface/pipelinestatedesc.hpp"

#include <span>
#include <string>


namespace render {

class IShaderReflection
{
public:
	virtual ~IShaderReflection() = default;
};

class IShaderCompiler
{
public:
	virtual std::unique_ptr<IShader> compile(
		EShaderType type,
		const std::string& source,
		std::wstring_view entryPoint,
		std::span<const ShaderDefine> defines) const = 0;

	virtual ~IShaderCompiler();

protected:
	IShaderCompiler(bool generate_debug_info);

protected:
	bool generate_debug_info_;
};

}  // namespace render
