#pragma once

#include "../pipelinestate.hpp"


struct ID3D12PipelineState;
struct ID3D12Device9;
struct ID3D12RootSignature;


namespace render {
class D3D12RenderInterface;
struct GraphicsCompiledShaders;
struct ComputeCompiledShaders;
}  // namespace render

namespace render {

class D3D12GraphicsPipelineState final : public IGraphicsPipelineState
{
public:
	static std::unique_ptr<D3D12GraphicsPipelineState> create(
		ID3D12Device9* device, const GraphicsPipelineDesc& desc, const GraphicsCompiledShaders& shaders);

	~D3D12GraphicsPipelineState() override = default;

	ID3D12PipelineState* getPipelineState() const { return pipeline_state_; }
	ID3D12RootSignature* getRootSignature() const { return root_signature_; }

protected:
	void setGpuDebugName() override;

private:
	D3D12GraphicsPipelineState(
		const GraphicsPipelineDesc& desc, ID3D12PipelineState* pipeline_state, ID3D12RootSignature* root_signature);

private:
	ID3D12PipelineState* pipeline_state_ = nullptr;
	ID3D12RootSignature* root_signature_ = nullptr;
};


class D3D12ComputePipelineState final : public IComputePipelineState
{
public:
	static std::unique_ptr<D3D12ComputePipelineState> create(
		ID3D12Device9* device, const ComputePipelineDesc& desc, const ComputeCompiledShaders& shaders);

	~D3D12ComputePipelineState() override = default;

	ID3D12PipelineState* getPipelineState() const { return pipeline_state_; }
	ID3D12RootSignature* getRootSignature() const { return root_signature_; }

protected:
	void setGpuDebugName() override;

private:
	D3D12ComputePipelineState(
		const ComputePipelineDesc& desc, ID3D12PipelineState* pipeline_state, ID3D12RootSignature* root_signature);

private:
	ID3D12PipelineState* pipeline_state_ = nullptr;
	ID3D12RootSignature* root_signature_ = nullptr;
};

class D3D12MeshPipelineState final : public IMeshPipelineState
{
	public:
	static std::unique_ptr<D3D12MeshPipelineState> create(
		ID3D12Device9* device, const MeshPipelineDesc& desc, const MeshCompiledShaders& shaders);

	~D3D12MeshPipelineState() override = default;

	ID3D12PipelineState* getPipelineState() const { return pipeline_state_; }
	ID3D12RootSignature* getRootSignature() const { return root_signature_; }

protected:
	void setGpuDebugName() override;

private:
	D3D12MeshPipelineState(const MeshPipelineDesc& desc, ID3D12PipelineState* pipeline_state, ID3D12RootSignature* root_signature);

private:
	ID3D12PipelineState* pipeline_state_ = nullptr;
	ID3D12RootSignature* root_signature_ = nullptr;
};

}  // namespace render
