#include "shadercompiler.hpp"


namespace render {

IShaderCompiler::~IShaderCompiler()
{
}


IShaderCompiler::IShaderCompiler(bool generate_debug_info)
	: generate_debug_info_(generate_debug_info)
{
}

}  // namespace render
