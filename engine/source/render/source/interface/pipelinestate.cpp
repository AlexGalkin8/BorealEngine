#include "pipelinestate.hpp"


namespace render {

IPipelineState::~IPipelineState()
{

}


IPipelineState::IPipelineState()
{

}

IGraphicsPipelineState::~IGraphicsPipelineState()
{
}


IGraphicsPipelineState::IGraphicsPipelineState(const GraphicsPipelineDesc& desc)
	: desc_(desc)
{
}


IComputePipelineState::~IComputePipelineState()
{
}


IComputePipelineState::IComputePipelineState(const ComputePipelineDesc& desc)
	: desc_(desc)
{
}


IMeshPipelineState::~IMeshPipelineState()
{
}


IMeshPipelineState::IMeshPipelineState(const MeshPipelineDesc& desc)
{
}

}  // namespace render