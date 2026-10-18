#pragma once

#include "../pipelinestate.hpp"


struct ID3D12PipelineState;
struct ID3D12Device9;
struct ID3D12RootSignature;


namespace render {

namespace internal {

class D3D12PipelineState
{
public:
	D3D12PipelineState(ID3D12PipelineState* pipeline_state, ID3D12RootSignature* root_signature);
	virtual ~D3D12PipelineState();

	ID3D12PipelineState* getPipelineState() const { return pipeline_state_; }
	ID3D12RootSignature* getRootSignature() const { return root_signature_; }

protected:
	void setGpuDebugNameImpl(const std::string& name);

protected:
	ID3D12PipelineState* pipeline_state_;
	ID3D12RootSignature* root_signature_;
};

}  // namespace internal


class D3D12GraphicsPipelineState final : public internal::D3D12PipelineState, public IGraphicsPipelineState
{
public:
	static std::unique_ptr<D3D12GraphicsPipelineState> create(
		ID3D12Device9* device, const GraphicsPipelineDesc& desc, const GraphicsCompiledShaders& shaders);

protected:
	void setGpuDebugName() override { setGpuDebugNameImpl(getName()); }

private:
	D3D12GraphicsPipelineState(const GraphicsPipelineDesc& desc, ID3D12PipelineState* pipeline_state, ID3D12RootSignature* root_signature);
};


class D3D12ComputePipelineState final : public internal::D3D12PipelineState, public IComputePipelineState
{
public:
	static std::unique_ptr<D3D12ComputePipelineState> create(
		ID3D12Device9* device, const ComputePipelineDesc& desc, const ComputeCompiledShaders& shaders);

protected:
	void setGpuDebugName() override { setGpuDebugNameImpl(getName()); }

private:
	D3D12ComputePipelineState(const ComputePipelineDesc& desc, ID3D12PipelineState* pipeline_state, ID3D12RootSignature* root_signature);
};


class D3D12MeshPipelineState final : public internal::D3D12PipelineState, public IMeshPipelineState
{
public:
	static std::unique_ptr<D3D12MeshPipelineState> create(
		ID3D12Device9* device, const MeshPipelineDesc& desc, const MeshCompiledShaders& shaders);

protected:
	void setGpuDebugName() override { setGpuDebugNameImpl(getName()); }

private:
	D3D12MeshPipelineState(const MeshPipelineDesc& desc, ID3D12PipelineState* pipeline_state, ID3D12RootSignature* root_signature);
};

}  // namespace render
