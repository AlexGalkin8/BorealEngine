#pragma once

#include "interface/common.h"

#include <vector>
#include <memory>


namespace render {

class IRenderInterface;
class IShaderReflection;


class IShader : public internal::RenderObject
{
public:
	IShader(std::vector<uint8_t>&& bytecode, std::unique_ptr<IShaderReflection>&& reflection);
	~IShader();

	const IShaderReflection& getReflection() const { return *shader_reflection; }

	const uint8_t* getBytecodeData() const { return shader_bytecode.data(); }
	const size_t getBytecodeSize() const { return shader_bytecode.size(); }

	bool empty() const { return shader_bytecode.empty(); }

private:
	void setGpuDebugName() override;

private:
	std::vector<uint8_t> shader_bytecode;
	std::unique_ptr<IShaderReflection> shader_reflection;
};

}  // namespace render
