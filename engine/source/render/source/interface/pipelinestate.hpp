#pragma once

#include <interface/common.h>

#include "pipelinestatedesc.hpp"

#include <memory>


namespace render {

class IPipelineState : public internal::RenderObject
{
public:

public:
	virtual ~IPipelineState();

protected:
	IPipelineState();

protected:
};


class IGraphicsPipelineState : public IPipelineState
{
public:

public:
	virtual ~IGraphicsPipelineState() override;

protected:
	IGraphicsPipelineState(const GraphicsPipelineDesc& desc);

protected:
	GraphicsPipelineDesc desc_;
};


class IComputePipelineState : public IPipelineState
{
public:

public:
	virtual ~IComputePipelineState() override;

protected:
	IComputePipelineState(const ComputePipelineDesc& desc);

protected:
	ComputePipelineDesc desc_;
};


class IMeshPipelineState : public IPipelineState
{
public:
	virtual ~IMeshPipelineState() override;

protected:
	IMeshPipelineState(const MeshPipelineDesc& desc);

protected:
	MeshPipelineDesc desc_;
};

}  // namespace render
