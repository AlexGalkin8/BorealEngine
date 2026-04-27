#include "shader.hpp"
#include "shadercompiler.hpp"


namespace render {

IShader::IShader(std::vector<uint8_t>&& bytecode, std::unique_ptr<IShaderReflection>&& reflection)
	: shader_bytecode(std::move(bytecode))
	, shader_reflection(std::move(reflection))
{
}


IShader::~IShader()
{
}

void IShader::setGpuDebugName()
{

}

}  // namespace render